//
// fileio.cpp: File handling (mainly disk related)
//
// by James Hammons
// (C) 2019 Underground Software
//

#include "fileio.h"

#include <stdlib.h>
#include <string.h>
#include "crc32.h"
#include "log.h"


uint8_t woz1Header[8] = { 'W', 'O', 'Z', '1', 0xFF, 0x0A, 0x0D, 0x0A };
uint8_t woz2Header[8] = { 'W', 'O', 'Z', '2', 0xFF, 0x0A, 0x0D, 0x0A };
uint8_t standardTMAP[160] = {
	0, 0, 0xFF, 1, 1, 1, 0xFF, 2, 2, 2, 0xFF, 3, 3, 3, 0xFF, 4, 4, 4, 0xFF,
	5, 5, 5, 0xFF, 6, 6, 6, 0xFF, 7, 7, 7, 0xFF, 8, 8, 8, 0xFF, 9, 9, 9, 0xFF,
	10, 10, 10, 0xFF, 11, 11, 11, 0xFF, 12, 12, 12, 0xFF, 13, 13, 13, 0xFF,
	14, 14, 14, 0xFF, 15, 15, 15, 0xFF, 16, 16, 16, 0xFF, 17, 17, 17, 0xFF,
	18, 18, 18, 0xFF, 19, 19, 19, 0xFF, 20, 20, 20, 0xFF, 21, 21, 21, 0xFF,
	22, 22, 22, 0xFF, 23, 23, 23, 0xFF, 24, 24, 24, 0xFF, 25, 25, 25, 0xFF,
	26, 26, 26, 0xFF, 27, 27, 27, 0xFF, 28, 28, 28, 0xFF, 29, 29, 29, 0xFF,
	30, 30, 30, 0xFF, 31, 31, 31, 0xFF, 32, 32, 32, 0xFF, 33, 33, 33, 0xFF,
	34, 34, 34, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF
};


//
// sizePtr is optional
//
uint8_t * ReadFile(const char * filename, uint32_t * sizePtr/*= NULL*/, uint32_t skip/*= 0*/)
{
	FILE * fp = fopen(filename, "rb");
	if (!fp) {
		return NULL;
	}

	if (fseek(fp, 0, SEEK_END) != 0)
	{
		fclose(fp);
		return NULL;
	}

	long fileSize = ftell(fp);
	if (fileSize <= 0 || (unsigned long)fileSize > UINT32_MAX || skip >= (uint32_t)fileSize ||
		fseek(fp, (long)skip, SEEK_SET) != 0)
	{
		fclose(fp);
		return NULL;
	}

	uint32_t size = (uint32_t)fileSize - skip;
	uint8_t * buffer = (uint8_t *)malloc(size);

	if (buffer == NULL)
	{
		fclose(fp);
		return NULL;
	}
 
	if (fread(buffer, 1, size, fp) != size)
	{
		free(buffer);
		fclose(fp);
		return NULL;
	}

	fclose(fp);

	if (sizePtr != NULL) {
		*sizePtr = size;
	}

	return buffer;
}


//
// This initializes the WOZ type 2 headers
//
void InitWOZ2Headers(WOZ2 & woz)
{
	// Set up header (leave CRC as 0 for now)
	memcpy(woz.magic, woz2Header, 8);
	woz.crc32 = 0;

	// INFO header
	memcpy(woz.infoTag, "INFO", 4);
	woz.infoSize = Uint32LE(60);
	woz.infoVersion = 2;
	woz.diskType = 1;
	woz.writeProtected = 0;
	woz.synchronized = 0;
	woz.cleaned = 1;
	memset(woz.creator, ' ', 32);
	memcpy(woz.creator, "Apple2 emulator v1.0.0", 22);
	woz.diskSides = 1;
	woz.bootSectorFmt = 1;
	woz.optimalBitTmg = 32;
	woz.largestTrack = Uint16LE(13);

	// TMAP header
	memcpy(woz.tmapTag, "TMAP", 4);
	woz.tmapSize = Uint32LE(160);

	// TRKS header
	memcpy(woz.trksTag, "TRKS", 4);
}


//
// This is used mainly to initialize blank disks and upconvert non-WOZ disks
//
uint8_t * InitWOZ(uint32_t * pSize/*= NULL*/)
{
	uint32_t size = 1536 + (35 * (13 * 512));
	uint8_t * data = (uint8_t *)malloc(size);
	if (data == NULL) {
		return NULL;
	}
	WOZ2 & woz = *((WOZ2 *)data);

	// Zero out WOZ image in memory
	memset(&woz, 0, size);

	// Set up headers
	InitWOZ2Headers(woz);
	memcpy(woz.tmap, standardTMAP, 160);
	woz.trksSize = Uint32LE(35 * (13 * 512));

	for(int i=0; i<35; i++)
	{
		woz.track[i].startingBlock = Uint16LE(3 + (i * 13));
		woz.track[i].blockCount = Uint16LE(13);
		woz.track[i].bitCount = Uint32LE(51200);
	}

	// META header (how to handle? prolly with a separate pointer)

	if (pSize)
		*pSize = size;

	return data;
}


uint8_t * UpconvertWOZ1ToWOZ2(uint8_t * woz1Data, uint32_t woz1Size, uint32_t * newSize)
{
	if (woz1Data == NULL || newSize == NULL || woz1Size < 256) {
		return NULL;
	}
	WOZ1 & woz1 = *((WOZ1 *)woz1Data);

	// First, figure out how large the new structure will be in comparison to
	// the old one...
	uint32_t numTracks = Uint32LE(woz1.trksSize) / sizeof(WOZ1Track);
	uint32_t metadataSize = woz1Size - (Uint32LE(woz1.trksSize) + 256);
	uint32_t trackStorageSize = numTracks * (13 * 512);
	if (metadataSize > UINT32_MAX - 0x600 - trackStorageSize) {
		return NULL;
	}

	// N.B.: # of blocks for each track will *always* be <= 13 for WOZ1
	*newSize = 0x600 + trackStorageSize + metadataSize;
	uint8_t * woz2Data = (uint8_t *)malloc(*newSize);
	if (woz2Data == NULL) {
		return NULL;
	}
	memset(woz2Data, 0, *newSize);

	WOZ2 & woz2 = *((WOZ2 *)woz2Data);
	InitWOZ2Headers(woz2);

	// Copy parts of INFO & TMAP chunks over
	memcpy(&woz2.diskType, &woz1.diskType, 36);
	memcpy(woz2.tmap, woz1.tmap, 160);
//note: should check the CRC32 integrity 1st before attempting to recreate it here...  (the CRC is written out when it's saved anyway, so no need to fuck with this right now)
	woz2.crc32 = 0;
	woz2.trksSize = Uint32LE(numTracks * (13 * 512));

	// Finally, copy over the tracks
	for(uint32_t i=0; i<numTracks; i++)
	{
		woz2.track[i].startingBlock = Uint16LE(3 + (i * 13));
		woz2.track[i].blockCount = Uint16LE(13);
		woz2.track[i].bitCount = woz1.track[i].bitCount;
		memcpy(woz2Data + ((3 + (i * 13)) * 512), woz1.track[i].bits, 6646);
	}

	// Finally, copy over the metadata
	memcpy(woz2Data + Uint32LE(woz2.trksSize) + 0x600,
		woz1Data + Uint32LE(woz1.trksSize) + 0x100, metadataSize);

	return woz2Data;
}


//
// Check WOZ type on the passed in contents (file loaded elsewhere).
// Returns type of WOZ if successful, 0 if not.
//
uint8_t CheckWOZType(const uint8_t * wozData, uint32_t wozSize)
{
	// Basic sanity checking
	if ((wozData == NULL) || (wozSize < 8))
		return 0;

	if (memcmp(wozData, woz1Header, 8) == 0)
		return 1;
	else if (memcmp(wozData, woz2Header, 8) == 0)
		return 2;

	return 0;
}


//
// Do basic sanity checks on the passed in contents (file loaded elsewhere).
// Returns true if successful, false on failure.
//
bool CheckWOZIntegrity(const uint8_t * wozData, uint32_t wozSize)
{
	// The common WOZ header and the fixed INFO/TMAP/TRKS headers occupy
	// 256 bytes.  Do not cast to either WOZ structure until the relevant
	// portion of the file has been shown to exist.
	if (wozData == NULL || wozSize < 256) {
		return false;
	}

//	if ((wozCRC != 0) && (wozCRC != crc))
	uint32_t storedCRC;
	memcpy(&storedCRC, wozData + 8, sizeof(storedCRC));
	storedCRC = Uint32LE(storedCRC);
	if (storedCRC != 0 && storedCRC != CRC32(wozData + 12, wozSize - 12))
	{
		WriteLog("FILEIO: Corrupted data found in WOZ. CRC32: %08X\n", storedCRC);
		return false;
	}
	if (storedCRC == 0) {
		WriteLog("FILEIO: Warning--WOZ file has no CRC...\n");
	}

	if (memcmp(wozData + 12, "INFO", 4) != 0 ||
		memcmp(wozData + 80, "TMAP", 4) != 0 ||
		memcmp(wozData + 248, "TRKS", 4) != 0)
	{
		return false;
	}

	uint32_t infoSize, tmapSize, trksSize;
	memcpy(&infoSize, wozData + 16, 4);
	memcpy(&tmapSize, wozData + 84, 4);
	memcpy(&trksSize, wozData + 252, 4);
	infoSize = Uint32LE(infoSize);
	tmapSize = Uint32LE(tmapSize);
	trksSize = Uint32LE(trksSize);
	if (infoSize != 60 || tmapSize != 160 || trksSize > wozSize - 256) {
		return false;
	}

	const uint8_t * tmap = wozData + 88;
	if (memcmp(wozData, woz1Header, 8) == 0)
	{
		uint32_t trackBytes = trksSize;
		if ((trackBytes % sizeof(WOZ1Track)) != 0 || trackBytes > wozSize - 256)
			return false;
		uint32_t trackCount = trackBytes / sizeof(WOZ1Track);
		if (trackCount > 160)
			return false;

		for (uint32_t i = 0; i < 160; ++i)
		{
			if (tmap[i] == 0xFF)
				continue;
			if (tmap[i] >= trackCount)
				return false;
			const uint8_t * track = wozData + 256 + ((uint32_t)tmap[i] * sizeof(WOZ1Track));
			uint16_t byteCount, bitCount;
			memcpy(&byteCount, track + 6646, 2);
			memcpy(&bitCount, track + 6648, 2);
			byteCount = Uint16LE(byteCount);
			bitCount = Uint16LE(bitCount);
			if (byteCount == 0 || byteCount > 6646 || bitCount == 0 || bitCount > (uint32_t)byteCount * 8)
				return false;
		}
	}
	else if (memcmp(wozData, woz2Header, 8) == 0)
	{
		// WOZ2 has 160 fixed-size track descriptors before track data.
		if (wozSize < 1536) {
			return false;
		}

		for (uint32_t i = 0; i < 160; ++i)
		{
			if (tmap[i] == 0xFF) {
				continue;
			}

			if (tmap[i] >= 160) {
				return false;
			}
			const uint8_t * desc = wozData + 256 + ((uint32_t)tmap[i] * 8);
			uint16_t startBlock, blockCount;
			uint32_t bitCount;
			memcpy(&startBlock, desc, 2);
			memcpy(&blockCount, desc + 2, 2);
			memcpy(&bitCount, desc + 4, 4);
			startBlock = Uint16LE(startBlock);
			blockCount = Uint16LE(blockCount);
			bitCount = Uint32LE(bitCount);
			uint32_t dataStart = (uint32_t)startBlock * 512;
			uint32_t dataSize = (uint32_t)blockCount * 512;
			if (startBlock < 3 || blockCount == 0 || bitCount == 0 ||
				bitCount > dataSize * 8 || dataStart > wozSize || dataSize > wozSize - dataStart)
			{
				return false;
			}
		}
	}
	else
	{
		return false;
	}

	WriteLog("FILEIO: Well formed WOZ file found\n");
	return true;
}


bool SaveWOZ(const char * filename, WOZ2 * woz, uint32_t size)
{
	if (filename == NULL || woz == NULL || size < 256) {
		return false;
	}

	// Set up CRC32 before writing
	woz->crc32 = Uint32LE(CRC32(woz->infoTag, size - 12));

	// META header (skip for now) (actually, should be in the disk[] image already)

	// Finally, write the damn image
	FILE * file = fopen(filename, "wb");

	if (file == NULL)
	{
		WriteLog("FILEIO: Failed to open image file '%s' for writing...\n", filename);
		return false;
	}

	bool success = (fwrite((uint8_t *)woz, 1, size, file) == size);
	if (fclose(file) != 0) {
		success = false;
	}
	if (!success)
	{
		WriteLog("FILEIO: Failed while writing image file '%s'.\n", filename);
		return false;
	}

	WriteLog("FILEIO: Successfully wrote image file '%s'...\n", filename);

	return true;
}
