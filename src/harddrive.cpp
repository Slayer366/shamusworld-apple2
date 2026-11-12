//
// Hard drive support
//
// by James Hammons
// (C) 2019 Underground Software
//
// This is done by emulating the Apple 2 High-Speed SCSI card.
//
// How it works:
//
// First 1K is the driver ROM, repeated four times.  After that, there are 31 1K
// chunks that are addressed in the $CC00-$CFFF address range; $C800-$CBFF is a
// 1K RAM space (8K static RAM, bank switched).
//

#include "harddrive.h"
#include "apple2.h"
#include "dis65c02.h"
#include "fileio.h"
#include "firmware/a2hs-scsi.h"
#include "log.h"
#include "mmu.h"
#include "settings.h"
#include "v65c02.h"		// For dumpDis...
#include <vector>


static uint8_t romBank = 0;
static uint8_t ramBank = 0;
static uint8_t deviceID = 7;
static bool dmaSwitch = false;
static uint8_t staticRAM[0x2000] = { 0 };
static uint8_t reg[16];

enum {
	DVM_DATA_OUT = 0,
	DVM_DATA_IN = 1,
	DVM_COMMAND = 2,
	DVM_STATUS = 3,
	DVM_MESSAGE_OUT = 6,
	DVM_MESSAGE_IN = 7,
	DVM_BUS_FREE = 8,
	DVM_ARBITRATE = 16,
	DVM_SELECT = 32
};

static bool DATA_BUS = false;
static bool DMA_MODE = false;
static bool BSY = false;
static bool ATN = false;
static bool SEL = false;
static bool ACK = false;
static bool RST = false;
static bool MSG = false;
static bool C_D = false;
static bool I_O = false;
static bool REQ = false;
static bool DEV_BSY = false;
static bool DRQ = false;
static bool DACK = false;
static uint8_t devMode = DVM_BUS_FREE;
static uint8_t cmdLength;
static uint8_t cmd[256];
static uint32_t bytesToSend;
static uint8_t * buf;
static uint32_t bufPtr;
static uint8_t response;
static uint8_t senseKey = 0;
static uint8_t senseASC = 0;
static uint8_t senseASCQ = 0;

// Clock of the last poll while a selection went unanswered (see HardDriveIsWaiting)
static uint64_t selectWaitClock = 0;
static bool selectWaitSeen = false;

// True while the firmware is stuck timing out on an empty SCSI ID.  A pure
// polling loop, so the emulator can safely fast-forward through it.
bool HardDriveIsWaiting(void)
{
	return selectWaitSeen && ((mainCPU.clock - selectWaitClock) < 4096);
}


static void SetCheckCondition(uint8_t key, uint8_t asc, uint8_t ascq = 0)
{
	response = 0x02;
	senseKey = key;
	senseASC = asc;
	senseASCQ = ascq;
}


// Keep static storage for SCSI data phase buffers instead of caching the whole disk
static uint8_t scsiSectorBuf[512 * 256]; // Maximum 256 blocks per single SCSI transfer command
static uint32_t activeLBA = 0;
static bool activeWrite = false;
static uint32_t hdSize = 0;
static uint32_t hdDataOffset = 0;
static FILE * hdFile = NULL;
static bool hdWritable = false;


static uint32_t ReadLE32(const uint8_t *p)
{
	return (uint32_t)p[0] | ((uint32_t)p[1] << 8)
		| ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}


static uint16_t ReadLE16(const uint8_t *p)
{
	return (uint16_t)((uint16_t)p[0] | ((uint16_t)p[1] << 8));
}


static bool LoadHardDriveImage(const char *filename)
{
	if (hdFile) {
		fclose(hdFile);
		hdFile = NULL;
	}
	hdWritable = false;
	hdSize = hdDataOffset = 0;
	bool canWrite = false;

	FILE *fp = fopen(filename, "rb+");
	if (fp) {
		canWrite = true;
	} else {
		fp = fopen(filename, "rb");
	}
	if (!fp) {
		return false;
	}
	if (fseek(fp, 0, SEEK_END) != 0) {
		fclose(fp);
		return false;
	}

	long fileLength = ftell(fp);

	if (fileLength <= 0 || (unsigned long)fileLength > 0xFFFFFFFFul || fseek(fp, 0, SEEK_SET) != 0) {
		fclose(fp);
		return false;
	}

	uint32_t offset = 0;
	uint32_t length = (uint32_t)fileLength;
	bool imageReadOnly = false;
	uint8_t header[64];

	size_t headerBytes = fread(header, 1, sizeof(header), fp);
	if (headerBytes >= 4 && memcmp(header, "2IMG", 4) == 0) {
		if (headerBytes < sizeof(header)) {
			fclose(fp);
			return false;
		}

		uint32_t headerSize = ReadLE16(header + 8);
		uint32_t imageFormat = ReadLE32(header + 12);
		uint32_t flags = ReadLE32(header + 16);
		uint32_t blockCount = ReadLE32(header + 20);
		offset = ReadLE32(header + 24);
		length = ReadLE32(header + 28);

		if (imageFormat != 1 || headerSize < 64 || headerSize > (uint32_t)fileLength) {
			fclose(fp);
			return false;
		}

		if (offset == 0) {
			offset = headerSize;
		}
		if (length == 0 && blockCount != 0 && blockCount <= 0xFFFFFFFFu / 512u) {
			length = blockCount * 512u;
		}
		if (offset < headerSize || offset > (uint32_t)fileLength || length == 0 ||
			length > (uint32_t)fileLength - offset || (length % 512) != 0 ||
			(blockCount != 0 && blockCount > 0xFFFFFFFFu / 512u) ||
			(blockCount != 0 && blockCount * 512u != length)) {
			fclose(fp);
			return false;
		}

		imageReadOnly = (flags & 0x80000000u) != 0;
	} else {
		if ((length % 512) != 0 || length == 0) {
			fclose(fp);
			return false;
		}
	}

	hdSize = length;
	hdDataOffset = offset;
	hdWritable = canWrite && !imageReadOnly;
	hdFile = fp;

	WriteLog("HD: Image data offset $%X, length $%X, %u blocks%s.\n",
		hdDataOffset, hdSize, hdSize / 512,
		imageReadOnly ? " (2MG write-protected)" : (hdWritable ? "" : " (host read-only)"));
	return true;
}


static bool PrepareRead(uint32_t lba, uint32_t blocks)
{
	buf = NULL;
	bufPtr = 0;
	bytesToSend = 0;

	if (!hdFile) {
		SetCheckCondition(0x02, 0x3A); // NOT READY: medium not present
		return false;
	}

	if (lba > hdSize / 512 || blocks > (hdSize / 512) - lba) {
		SetCheckCondition(0x05, 0x21); // ILLEGAL REQUEST: LBA out of range
		return false;
	}

	if (blocks > 256) {
		SetCheckCondition(0x05, 0x24); // ILLEGAL REQUEST: invalid field in CDB
		return false;
	}

	if (blocks == 0) {
		return true;
	}

	uint64_t filePos = (uint64_t)hdDataOffset + ((uint64_t)lba * 512);
	if (filePos > 0x7FFFFFFF || fseek(hdFile, (long)filePos, SEEK_SET) != 0 ||
			fread(scsiSectorBuf, 1, blocks * 512, hdFile) != blocks * 512) {
				SetCheckCondition(0x03, 0x11); // MEDIUM ERROR: unrecovered read error
				return false;
	}

	buf = scsiSectorBuf;
	bytesToSend = blocks * 512;
	return true;
}


static inline void SetNextState(uint8_t state)
{
	devMode = state;
	MSG = (state & 0x04 ? true : false);
	C_D = (state & 0x02 ? true : false);
	I_O = (state & 0x01 ? true : false);
	cmdLength = 0;
}


static void RunDevice(void)
{
	// These are SCSI messages sent in response to certain commands
	static uint8_t readCapacity[8] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x00 };

	// Standard 36-byte SCSI INQUIRY data. Keep the historical vendor/model
	// identity while supplying the fields and padding expected by SCSI-2.
	static uint8_t inquireData[36] = { 0x00, 0x00, 0x02, 0x02, 31, 0x00, 0x00, 0x00, 'S', 'E', 'A', 'G', 'A', 'T', 'E', ' ', '3', '1', '3', '3', '7', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', '0', '0', '0', '1' };

	// MODE SENSE(6), error-recovery page (page 1), with no block descriptor.
	static uint8_t modeSenseData[16] = { 0x0F, 0x00, 0x00, 0x00, 0x01, 0x0A, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
	static uint8_t senseData[18] = { 0x70, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0A, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };

	if (RST)
	{
		devMode = DVM_BUS_FREE;
		DEV_BSY = false;
		// A bus reset aborts any command and any outstanding DMA handshake.
		// Do not leave a stale request/data phase active after the reset.
		REQ = false;
		DRQ = false;
		DACK = false;
		buf = NULL;
		bufPtr = 0;
		bytesToSend = 0;
		activeWrite = false;
		cmdLength = 0;
		response = 0x00;
		return;
	}

	switch (devMode)
	{
	case DVM_BUS_FREE:
		// We never initiate, so this we don't worry about whether or not the
		// bus is free.
	case DVM_ARBITRATE:
		// Likewise, we don't arbitrate either.
		break;

	case DVM_SELECT:
		// If we're in Selection phase, see if our ID is on the bus, and, if so,
		// go on to the next phase (since the Target drives the phase dance).
		if ((reg[0] & 0x40) && DATA_BUS)
		{
			DEV_BSY = true;

			// Preset response code to "Good"
			response = 0x00;
			activeWrite = false;
			buf = NULL;
			bufPtr = 0;
			bytesToSend = 0;
			DRQ = DACK = false;
			REQ = false;

			if (ATN)
				SetNextState(DVM_MESSAGE_OUT);
			else
				// If no ATN is asserted, go to COMMAND?  Dunno, the firmware
				// doesn't ever go there; it *always* starts with MESSAGE OUT.
				SetNextState(DVM_COMMAND);
		}

		break;

	case DVM_DATA_OUT:
//WriteLog("   >>> DATA OUT PHASE (bts=%u)\n", bytesToSend);
		if (bytesToSend == 0)
		{
			REQ = false;
			SetNextState(DVM_STATUS);
			buf = NULL;
			break;
		}
		if (!ACK)
			REQ = true;

		if (DMA_MODE)
		{
			if (!DACK)
			{
				DRQ = true;
			}
			else if (DRQ && DACK)
			{
				if (buf)
				{
					buf[bufPtr] = reg[0];
				}

				DRQ = DACK = false;
				bytesToSend--;
				bufPtr++;

				if (bytesToSend == 0)
				{
					// Flush block payload structure entirely cleanly at the end of the transaction frame
					if (activeWrite && hdWritable && hdFile)
					{
						uint64_t filePos = (uint64_t)hdDataOffset + ((uint64_t)activeLBA *512);
						if (filePos <= 0x7FFFFFFF && fseek(hdFile, (long)filePos, SEEK_SET) == 0)
						{
							if (fwrite(scsiSectorBuf, 1, bufPtr, hdFile) != bufPtr) {
								SetCheckCondition(0x03, 0x0C); // MEDIUM ERROR: write error
							}
							fflush(hdFile);
						} else {
							SetCheckCondition(0x03, 0x0C);
						}
					}
					activeWrite = false;
					WriteLog("HD: DATA OUT complete (%u bytes).\n", bufPtr);
					REQ = false;
					SetNextState(DVM_STATUS);
					buf = NULL;
				}
			}
		}

		break;

	case DVM_DATA_IN:
//WriteLog("   >>> DATA IN PHASE (bts=%u)\n", bytesToSend);
		if (bytesToSend == 0)
		{
			REQ = false;
			SetNextState(DVM_STATUS);
			buf = NULL;
			break;
		}
		if (!ACK)
			REQ = true;

		if (DMA_MODE)
		{
			if (!DACK)
			{
				// If there's no buffer set up, send zeroes...
				reg[6] = (buf == NULL ? 0 : buf[bufPtr]);
				DRQ = true;
			}
			else if (DRQ && DACK)
			{
				DRQ = DACK = false;
				bytesToSend--;
				bufPtr++;

				if (bytesToSend == 0)
				{
					WriteLog("HD: DATA IN complete (%u bytes).\n", bufPtr);
					REQ = false;
					SetNextState(DVM_STATUS);
					buf = NULL;
				}
			}
		}

		break;

	case DVM_COMMAND:
	{
		if (!ACK)
			REQ = true;
		else if (REQ && ACK)
		{
			cmd[cmdLength++] = reg[0];
			REQ = false;
		}

		uint8_t cmdType = (cmd[0] & 0xE0) >> 5;

		if ((cmdType == 0) && (cmdLength == 6))
		{
			// Handle "Test Unit Ready" command
			if (cmd[0] == 0)
			{
				//WriteLog("HD: Received command TEST UNIT READY\n");
				if ((cmd[1] & 0xE0) != 0) {
					SetCheckCondition(0x05, 0x25); // Logical unit not supported
				} else if (!hdFile) {
					SetCheckCondition(0x02, 0x3A); // Medium not present
				}
				SetNextState(DVM_STATUS);
			}
			// Handle "Request Sense" command
			else if (cmd[0] == 0x03)
			{
				//WriteLog("HD: Received command REQUEST SENSE [%02X %02X %02X %02X %02X %02X]\n", cmd[0], cmd[1], cmd[2], cmd[3], cmd[4], cmd[5]);
				memset(senseData, 0, sizeof(senseData));
				senseData[0] = 0x70; // Current errors, fixed format
				senseData[2] = senseKey;
				senseData[7] = 10;
				senseData[12] = senseASC;
				senseData[13] = senseASCQ;
				buf = senseData;
				bufPtr = 0;
				bytesToSend = cmd[4] < sizeof(senseData) ? cmd[4] : sizeof(senseData);
				response = 0x00; // REQUEST SENSE reports the saved condition as data
				SetNextState(bytesToSend ? DVM_DATA_IN : DVM_STATUS);
				// Sense data is cleared after it has been copied into the response.
				senseKey = senseASC = senseASCQ = 0;
			}
			// Handle "Read" (6) command
			else if (cmd[0] == 0x08)
			{
				//WriteLog("HD: Received command READ(6) [%02X %02X %02X %02X %02X %02X]\n", cmd[0], cmd[1], cmd[2], cmd[3], cmd[4], cmd[5]);
				uint32_t blocks = cmd[4] ? cmd[4] : 256;
				uint32_t lba = ((cmd[1] & 0x1F) << 16) | (cmd[2] << 8) | cmd[3];
				if ((cmd[1] & 0xE0) != 0) {
					SetCheckCondition(0x05, 0x25); // Logical unit not supported
					SetNextState(DVM_STATUS);
				} else if (PrepareRead(lba, blocks)) {
					//WriteLog("HD: READ(6) LBA %u, %u blocks (%u bytes).\n", lba, blocks, bytesToSend);
					SetNextState(blocks ? DVM_DATA_IN : DVM_STATUS);
				} else {
					WriteLog("HD: READ(6) rejected: LBA %u, %u blocks.\n", lba, blocks);
					SetNextState(DVM_STATUS);
				}
			}
			// Handle "Inquire" command
			else if (cmd[0] == 0x12)
			{
				//WriteLog("HD: Received command INQUIRE [%02X %02X %02X %02X %02X %02X]\n", cmd[0], cmd[1], cmd[2], cmd[3], cmd[4], cmd[5]);
				buf = inquireData;
				bufPtr = 0;
				bytesToSend = cmd[4] < sizeof(inquireData) ? cmd[4] : sizeof(inquireData);

				// Preserve the existing unsupported-LUN behavior, but report
				// a sense code that REQUEST SENSE can explain to the firmware.
				if ((cmd[1] & 0xE0) != 0) {
					SetCheckCondition(0x05, 0x25); // Logical unit not supported
				}
				SetNextState(bytesToSend ? DVM_DATA_IN : DVM_STATUS);
			}
			// Handle "Mode Select" command
			else if (cmd[0] == 0x15)
			{
				//WriteLog("HD: Received command MODE SELECT [%02X %02X %02X %02X %02X %02X]\n", cmd[0], cmd[1], cmd[2], cmd[3], cmd[4], cmd[5]);
				activeWrite = false;
				buf = NULL;
				bufPtr = 0;
				bytesToSend = cmd[4];

				if (bytesToSend) {
					SetNextState(DVM_DATA_OUT);
				} else {
					SetNextState(DVM_STATUS);
				}
			}
			// Handle "Mode Sense" command
			else if (cmd[0] == 0x1A)
			{
				//WriteLog("HD: Received command MODE SENSE [%02X %02X %02X %02X %02X %02X]\n", cmd[0], cmd[1], cmd[2], cmd[3], cmd[4], cmd[5]);
				if ((cmd[1] & 0xE0) != 0) {
					SetCheckCondition(0x05, 0x25);
					SetNextState(DVM_STATUS);
				} else {
					uint8_t pageCode = cmd[2] & 0x3F;
					modeSenseData[0] = 0x0F;
					modeSenseData[2] = hdWritable ? 0x00 : 0x80; // WP bit
					if (pageCode == 0x01 || pageCode == 0x3F) {
						buf = modeSenseData;
						bufPtr = 0;
						bytesToSend = cmd[4] < sizeof(modeSenseData) ? cmd[4] : sizeof(modeSenseData);
					} else {
						buf = modeSenseData;
						bufPtr = 0;
						bytesToSend = cmd[4] < 4 ? cmd[4] : 4;
						modeSenseData[0] = 0x03; // Header only: no requested page
					}
					SetNextState(bytesToSend ? DVM_DATA_IN : DVM_STATUS);
				}
			}
			else
			{
				WriteLog("HD: Received unhandled 6 command [%02X %02X %02X %02X %02X %02X]\n", cmd[0], cmd[1], cmd[2], cmd[3], cmd[4], cmd[5]);
				// Return failure...
				SetCheckCondition(0x05, 0x20); // ILLEGAL REQUEST: invalid command
				SetNextState(DVM_STATUS);
			}
		}
		else if (((cmdType == 1) || (cmdType == 2)) && (cmdLength == 10))
		{
			// Handle "Read Capacity" command
			if (cmd[0] == 0x25)
			{
//				WriteLog("HD: Received command READ CAPACITY [%02X %02X %02X %02X %02X %02X %02X %02X %02X %02X]\n", cmd[0], cmd[1], cmd[2], cmd[3], cmd[4], cmd[5], cmd[6], cmd[7], cmd[8], cmd[9]);
//				bytesToSend = 8;//it's always 8...//cmd[4];
				// N.B.: We need to hook this up to the actual emulated HD size...
//				buf = readCapacity;
//				bufPtr = 0;
				if ((cmd[1] & 0xE0) != 0)
				{
					SetCheckCondition(0x05, 0x25); // Logical unit not supported
					SetNextState(DVM_STATUS);
				}
				else if (!hdFile || hdSize < 512)
				{
					SetCheckCondition(0x02, 0x3A); // Medium not present
					SetNextState(DVM_STATUS);
				}
				else
				{
					uint32_t lastLBA = hdSize / 512 - 1;
					readCapacity[0] = (uint8_t)(lastLBA >> 24);
					readCapacity[1] = (uint8_t)(lastLBA >> 16);
					readCapacity[2] = (uint8_t)(lastLBA >> 8);
					readCapacity[3] = (uint8_t)lastLBA;
					SetNextState(DVM_DATA_IN);
					bytesToSend = 8;
					buf = readCapacity;
					bufPtr = 0;
				}
			}
			// Handle "Read" (10) command
			else if (cmd[0] == 0x28)
			{
				//WriteLog("HD: Received command READ(10) [%02X %02X %02X %02X %02X %02X %02X %02X %02X %02X]\n", cmd[0], cmd[1], cmd[2], cmd[3], cmd[4], cmd[5], cmd[6], cmd[7], cmd[8], cmd[9]);
				// Drive next phase
				uint32_t blocks = ((uint32_t)cmd[7] << 8) | cmd[8];
				uint32_t lba = ((uint32_t)cmd[2] << 24) | ((uint32_t)cmd[3] << 16) | ((uint32_t)cmd[4] << 8) | cmd[5];
				if ((cmd[1] & 0xE0) != 0) {
					SetCheckCondition(0x05, 0x25);
					SetNextState(DVM_STATUS);
				} else if (PrepareRead(lba, blocks)) {
					//WriteLog("HD: READ(10) LBA %u, %u blocks (%u bytes).\n", lba, blocks, bytesToSend);
					SetNextState(blocks ? DVM_DATA_IN : DVM_STATUS);
				} else {
					WriteLog("HD: READ(10) rejected: LBA %u, %u blocks.\n", lba, blocks);
					SetNextState(DVM_STATUS);
				}
			}
			// Handle "Write" (10) command
			else if (cmd[0] == 0x2A)
			{
				WriteLog("HD: Received command WRITE(10) [%02X %02X %02X %02X %02X %02X %02X %02X %02X %02X]\n", cmd[0], cmd[1], cmd[2], cmd[3], cmd[4], cmd[5], cmd[6], cmd[7], cmd[8], cmd[9]);
				// Drive next phase
				uint32_t blocks = ((uint32_t)cmd[7] << 8) | cmd[8];
				uint32_t lba = ((uint32_t)cmd[2] << 24) | ((uint32_t)cmd[3] << 16) | ((uint32_t)cmd[4] << 8) | cmd[5];
				if ((cmd[1] & 0xE0) != 0) {
					SetCheckCondition(0x05, 0x25);
					SetNextState(DVM_STATUS);
				}
				else if (!hdFile) {
					SetCheckCondition(0x02, 0x3A); // Medium not present
					SetNextState(DVM_STATUS);
				}
				else if (!hdWritable) {
					SetCheckCondition(0x07, 0x27); // DATA PROTECT: write protected
					SetNextState(DVM_STATUS);
				}
				else if (lba > hdSize / 512 || blocks > (hdSize / 512) - lba) {
					SetCheckCondition(0x05, 0x21);
					SetNextState(DVM_STATUS);
				}
				else if (blocks > 256) {
					SetCheckCondition(0x05, 0x24);
					SetNextState(DVM_STATUS);
				}
				else if (blocks == 0) {
					activeWrite = false;
					SetNextState(DVM_STATUS);
				}
				else
				{
					WriteLog("HD: WRITE(10) LBA %u, %u blocks (%u bytes).\n", lba, blocks, blocks * 512);
					activeLBA = lba;
					activeWrite = true;
					buf = scsiSectorBuf;
					bufPtr = 0;
					bytesToSend = blocks * 512;
					SetNextState(DVM_DATA_OUT);
				}
			}
			else
			{
				WriteLog("HD: Received unhandled 10 command [%02X %02X %02X %02X %02X %02X %02X %02X %02X %02X]\n", cmd[0], cmd[1], cmd[2], cmd[3], cmd[4], cmd[5], cmd[6], cmd[7], cmd[8], cmd[9]);
				// Return failure...
				SetCheckCondition(0x05, 0x20); // ILLEGAL REQUEST: invalid command
				SetNextState(DVM_STATUS);
			}
		}
		else if ((cmdType == 5) && (cmdLength == 12))
		{
			WriteLog("HD: Received unhandled 12 command [%02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X]\n", cmd[0], cmd[1], cmd[2], cmd[3], cmd[4], cmd[5], cmd[6], cmd[7], cmd[8], cmd[9], cmd[10], cmd[11]);
			// Return failure...
			SetCheckCondition(0x05, 0x20); // ILLEGAL REQUEST: invalid command
			SetNextState(DVM_STATUS);
		}
		else if (((cmdType == 3) && (cmdLength == 6)) || ((cmdType == 4) && (cmdLength == 16)) ||
						(((cmdType == 6) || (cmdType == 7)) && (cmdLength == 10)))
		{
			// Reserved/vendor-specific command groups are not implemented.
			// Consume the complete CDB and return CHECK CONDITION rather than
			// remaining in COMMAND phase indefinitely.
			WriteLog("HD: Received unsupported SCSI command group %u, opcode $%02X.\n", cmdType, cmd[0]);
			SetCheckCondition(0x05, 0x20); // ILLEGAL REQUEST: invalid command
			SetNextState(DVM_STATUS);
		}

		break;
	}

	case DVM_STATUS:
		if (!ACK)
		{
			reg[0] = response;
			REQ = true;
		}
		else if (REQ && ACK)
		{
			REQ = false;
			SetNextState(DVM_MESSAGE_IN);
		}

		break;

	case DVM_MESSAGE_OUT:
		if (!ACK)
			REQ = true;
		if (REQ && ACK)
		{
//			WriteLog("HD: Write to target value $%02X\n", reg[0]);
			REQ = false;
			SetNextState(DVM_COMMAND);
		}

		break;

	case DVM_MESSAGE_IN:
		if (!ACK)
		{
			// Return appropriate response
			// MESSAGE IN phase: COMMAND COMPLETE.
			// This is a message byte, not the SCSI status byte.
			reg[0] = 0x00;
			REQ = true;
		}
		else if (REQ && ACK)
		{
			REQ = false;
			DEV_BSY = false;
			SetNextState(DVM_BUS_FREE);
		}

		break;
	}
}


static uint8_t SlotIOR(uint16_t address)
{
	// This should prolly go somewhere else...
	RunDevice();

	uint8_t response = reg[address & 0x0F];

	switch (address & 0x0F)
	{
		case 0x00:
			// (RO) Current SCSI Data register
			break;
		case 0x01:
			// Initiator Command register.  Bits, from hi to lo:
			// ASS. /RST, AIP, LA, ASS. /ACK, A./BSY, A./SEL, A./ATN, DATA BUS

			// Simulate ARBITRATE signal
			if (reg[2] & 0x01)
				response |= 0x40;

			break;
		case 0x02:
			// Mode register (chip control)
			break;
		case 0x03:
			// Target Command register (SCSI bus info xfer phase)
			break;
		case 0x04:
			// (RO) Current SCSI Bus Status register:  Bits from hi to lo:
			// /RST, /BSY, /REQ, /MSG, /C/D, /I/O, /SEL, /DBP
/*if (((mainCPU.pc != 0xCD7C) && (mainCPU.pc != 0xCD5F)) || (romBank != 16))
	WriteLog("  [%02X %02X %02X %02X %02X %02X %02X %02X] [$C81F=$%02X $C80D=$%02X $C80A=$%02X $C887=$%02X $C806=$%02X $C88F=$%02X $C8EC=$%02X $4F=$%02X]\n", reg[0], reg[1], reg[2], reg[3], reg[4], reg[5], reg[6], reg[7], staticRAM[0x1F], staticRAM[0x0D], staticRAM[0x0A], staticRAM[0x87], staticRAM[0x06], staticRAM[0x8F], staticRAM[0xEC], ram[0x4F]);//*/

			response = (RST ? 0x80 : 0) | (BSY | DEV_BSY ? 0x40 : 0) | (REQ ? 0x20 : 0) | (MSG ? 0x10 : 0) | (C_D ? 0x08 : 0) | (I_O ? 0x04 : 0) | (SEL ? 0x02 : 0);

			// The firmware is polling for a target to answer a selection.  A target
			// that exists answers within a poll or two; an empty SCSI ID never does.
			if (SEL && !(BSY || DEV_BSY)) {
				selectWaitClock = mainCPU.clock;
				selectWaitSeen = true;
			}
			break;
		case 0x05:
		{
			// (RO) Bus and Status register
			response = (ACK ? 0x01 : 0) | (ATN ? 0x02 : 0) | (DRQ ? 0x40 : 0);
			uint8_t tgtMode = (MSG ? 0x04 : 0) | (C_D ? 0x02 : 0) | (I_O ? 0x01 : 0);

			if ((reg[3] & 0x07) == tgtMode)
				response |= 0x08;

			break;
		}
		case 0x06:
			// (RO) Input Data register (read from from SCSI bus)
			if (DRQ)
				DACK = true;

			break;
		case 0x07:
			// (RO) Reset Parity/Interrupt
			// Resets PARITY ERR (bit 6), IRQ (bit 5), BUSY ERROR (bit 3) in
			// register 5 (Bus & Status)
			break;
		case 0x0C:
			response = 0x10 | (dmaSwitch ? 0x40 : 0);
			break;
		case 0x0E:
			response = romBank | (deviceID << 5);
			break;
	}

#if 0
	char SCSIName[16][256] = {
		"(RO) Current SCSI Data",
		"Initiator Command",
		"Mode",
		"Target Command",
		"(RO) Current SCSI Bus Status",
		"(RO) Bus and Status",
		"(RO) Input Data",
		"(RO) Reset Parity/Interrupt",
		"DMA Address LO",
		"DMA Address HI",
		"DMA Count LO",
		"DMA Count HI",
		"$C",
		"$D",
		"Bank/SCSI ID",
		"$F"
	};

	if (((mainCPU.pc != 0xCD7C) && (mainCPU.pc != 0xCD5F)) || (romBank != 16))
		WriteLog("HD Slot I/O read %s ($%02X <- $%X, PC=%04X:%u)\n", SCSIName[address & 0x0F], response, address & 0x0F, mainCPU.pc, romBank);
#endif

	return response;
}


static void SlotIOW(uint16_t address, uint8_t byte)
{
	switch (address & 0x0F)
	{
		case 0x00:
			// (WO) Output Data register (data sent over SCSI bus)
			if (DRQ)
				DACK = true;

			break;
		case 0x01:
			// Initiator Command register.  Bits, from hi to lo:
			// ASS. /RST, AIP, LA, ASS. /ACK, A./BSY, A./SEL, A./ATN, DATA BUS
			DATA_BUS = (byte & 0x01 ? true : false);
			ATN = (byte & 0x02 ? true : false);
			SEL = (byte & 0x04 ? true : false);
			BSY = (byte & 0x08 ? true : false);
			ACK = (byte & 0x10 ? true : false);
			RST = (byte & 0x80 ? true : false);

			if (!(SEL || BSY || DEV_BSY))
				devMode = DVM_BUS_FREE;

			if (SEL && (devMode == DVM_ARBITRATE))
				devMode = DVM_SELECT;

			break;
		case 0x02:
			// Mode register (chip control)
			if ((byte & 0x01) && (devMode == DVM_BUS_FREE))
				devMode = DVM_ARBITRATE;

			// Dma ReQuest is reset here (as well as by hitting a pin)
			DMA_MODE = (byte & 0x02 ? true : false);

			if (!DMA_MODE)
				DRQ = DACK = false;

			break;
		case 0x03:
			// Target Command register (SCSI bus info xfer phase)
			break;
		case 0x04:
			// (WO) Select Enable register
			break;
		case 0x05:
			// (WO) Start DMA Send (initiates DMA send)
			DRQ = true;
			break;
		case 0x06:
			// (WO) Start DMA Target Receive (initiate DMA receive--tgt mode)
			DRQ = true;
			break;
		case 0x07:
			// (WO) Start DMA Initiator Receive (initiate DMA receive--ini mode)
			DRQ = true;
			break;
		case 0x08:
			// Lo byte of DMA address?
			break;
		case 0x09:
			// Hi byte of DMA address?
			break;
		case 0x0A:
			// 2's complement of lo byte of transfer amount?
			break;
		case 0x0B:
			// 2's complement of hi byte of transfer amount?
			break;
		case 0x0C:
			// Control/status register?
			break;
		case 0x0D:
			// ???
			break;
		case 0x0E:
			// Bottom 5 bits of $E set the ROM bank
			romBank = byte & 0x1F;
			break;
		case 0x0F:
			// Bottom 3 bits of $F set the RAM bank
			ramBank = byte & 0x07;
			break;
	}

	reg[address & 0x0F] = byte;

#if 0
	char SCSIName[16][256] = {
		"(WO) Output Data",
		"Initiator Command",
		"Mode",
		"Target Command",
		"(WO) Select Enable",
		"(WO) Start DMA Send",
		"(WO) Start DMA Target Receive",
		"(WO) Start DMA Initiator Receive",
		"DMA Address LO",
		"DMA Address HI",
		"DMA Count LO",
		"DMA Count HI",
		"$C",
		"$D",
		"Bank/SCSI ID",
		"$F"
	};
	char SCSIPhase[11][256] = { "DATA OUT", "DATA IN", "COMMAND", "STATUS", "ERR4", "ERR5", "MESSAGE OUT", "MESSAGE IN", "BUS FREE", "ARBITRATE", "SELECT" };


	WriteLog("HD Slot I/O write %s ($%02X -> $%X, PC=%04X:%u) [%s]\n", SCSIName[address & 0x0F], byte, address & 0x0F, mainCPU.pc, romBank, SCSIPhase[devMode]);

	if ((address & 0x0F) == 0x0E)
	{
		if (mainCPU.pc == 0xC78B)
		{
			uint16_t sp = mainCPU.sp;
			uint16_t pc = ram[0x100 + sp + 1] | (ram[0x100 + sp + 2] << 8);
			WriteLog("   *** Returning to bank %u, $%04X\n", romBank, pc + 1);
		}
		else if (mainCPU.pc == 0xC768)
		{
			WriteLog("   *** Calling to bank %u:%u\n", mainCPU.a, (mainCPU.y & 0xE0) >> 5);
		}

		WriteLog("  [%02X %02X %02X %02X %02X %02X %02X %02X] [$C81F=$%02X $C80D=$%02X $C80A=$%02X $C887=$%02X $C806=$%02X $C88F=$%02X $C8EC=$%02X $4F=$%02X]\n", reg[0], reg[1], reg[2], reg[3], reg[4], reg[5], reg[6], reg[7], staticRAM[0x1F], staticRAM[0x0D], staticRAM[0x0A], staticRAM[0x87], staticRAM[0x06], staticRAM[0x8F], staticRAM[0xEC], ram[0x4F]);
	}
#endif

	// This should prolly go somewhere else...
	RunDevice();
}


static uint8_t SlotROM(uint16_t address)
{
	return a2hsScsiROM[address];
}


// The firmware's boot code reads block 0 using its own internal parameter
// convention, then hands control to the boot sector with a "JMP $0801" at
// $CDDD (ROM bank 11).  Boot ROMs for ProDOS block devices normally leave the
// *ProDOS* driver call that read block 0 in zero page ($42 command = READ,
// $43 unit, $44/$45 buffer, $46/$47 block), and compact boot sectors rely  on
// that: they only set the block number and buffer high byte before calling
// the driver again.  This card's firmware leaves $42 = 0 (STATUS) instead, so
// such sectors loop forever on status calls. Reproduce the conventional state
// just before the hand-off.
#define BOOT_HANDOFF_ADDR		(0xCDDD - 0xC800)
#define BOOT_HANDOFF_BANK		11

static void SetProDOSBootParameters(void)
{
	AppleWriteMem(0x42, 0x01);				// Command: READ
	AppleWriteMem(0x43, mainCPU.x);			// Unit: slot << 4, drive 1 (X at hand-off)
	AppleWriteMem(0x44, 0x00);				// Buffer: $0800
	AppleWriteMem(0x45, 0x08);
	AppleWriteMem(0x46, 0x00);				// Block: 0
	AppleWriteMem(0x47, 0x00);
	WriteLog("HD: Boot hand-off: left ProDOS READ call in $42-$47 (unit $%02X) for the boot sector.\n", mainCPU.x);
}


static uint8_t SlotIOExtraR(uint16_t address)
{
	if (address < 0x400) {
		return staticRAM[(ramBank * 0x400) + address];
	}

	// About to execute the firmware's JMP $0801 (the byte at $CDDD is the opcode)
	if ((address == BOOT_HANDOFF_ADDR) && (romBank == BOOT_HANDOFF_BANK)) {
		SetProDOSBootParameters();
	}

	return a2hsScsiROM[(romBank * 0x400) + address - 0x400];
}


static void SlotIOExtraW(uint16_t address, uint8_t byte)
{
	if (address < 0x400)
		staticRAM[(ramBank * 0x400) + address] = byte;
	else {
		// The upper 1 KB is ROM; writes are ignored by hardware.
		//WriteLog("HD: Unhandled HD 1K ROM write ($%02X) @ $C%03X...\n", byte, address + 0x800);

//		if ((mainCPU.pc == 0xCDDD) && (romBank == 11))
//			dumpDis = true;
	}
}


bool InstallHardDrive(uint8_t slot, const char *imageOverride, bool suppressConfiguredImage)
{
	// The card is only installed once there is an image for it to serve.  An
	// empty SCSI card still gets booted first by the Apple //e's slot scan (it
	// searches from slot 7 down), and its firmware spends about 3.5 seconds of
	// emulated time timing out on every empty SCSI ID before giving up and letting
	// the floppy in slot 6 boot.  Without the card, that wait simply isn't there.
	if (suppressConfiguredImage) {
		WriteLog("HD: Configured hard disk suppressed by command-line floppy image.\n");
		return true;
	}

	const char *configuredImage = settings.hd[0];

	if (!imageOverride && settings.hd[1][0])
		WriteLog("HD: harddrive2 ('%s') is configured, but a second hard drive is not emulated yet; ignoring it.\n", settings.hd[1]);

	const char *imageName = imageOverride ? imageOverride : configuredImage;
	if (!imageName || !imageName[0]) {
		WriteLog("HD: No hard drive image configured.\n");
		return true;
	}

	char filename[(MAX_PATH * 2) + 2];
	if (imageOverride || strchr(imageName, '/') || strchr(imageName, '\\') ||
		(imageName[0] && imageName[1] == ':')) {
		snprintf(filename, sizeof(filename), "%s", imageName);
	} else {
		snprintf(filename, sizeof(filename), "%s%s", settings.disksPath, imageName);
	}

	if (!LoadHardDriveImage(filename)) {
		WriteLog("HD: Could not load hard drive image '%s'.\n", filename);
		return false;
	}

	WriteLog("HD: Loaded '%s', %u bytes (%u blocks).\n", filename, hdSize, hdSize / 512);
	SlotData hd = { SlotIOR, SlotIOW, SlotROM, 0, SlotIOExtraR, SlotIOExtraW };
	InstallSlotHandler(slot, &hd);
	return true;
}
