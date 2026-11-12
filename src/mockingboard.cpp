//
// Mockingboard support
//
// by James Hammons
// (C) 2018 Underground Software
//
// NOTES:
// bit 7 = L/R channel select (AY chip 1 versus AY chip 2)
//         0 = Left, 1 = Right
//
// Reg. B is connected to BC1, BDIR, RST' (bits 0, 1, 2)
//
// Left VIA IRQ line is tied to 6502 IRQ line
// Rght VIA IRQ line is tied to 6502 NMI line
//


#include "mockingboard.h"
#include "apple2.h"
#include "mmu.h"


MOCKINGBOARD mb[2];

// Recent Mockingboard register activity keeps disk turbo off (so music keeps
// its tempo).  An idle board does not hold turbo back.
static uint64_t lastMBActivityCycle = 0;
static bool mbActivitySeen = false;
static const uint64_t MB_ACTIVITY_TIMEOUT = 102048;	// 0.1 seconds at 1.0204832 MHz

static void NoteMBActivity(void)
{
	lastMBActivityCycle = mainCPU.clock;
	mbActivitySeen = true;
}

bool MBIsActive(void)
{
	return mbActivitySeen && ((mainCPU.clock - lastMBActivityCycle) < MB_ACTIVITY_TIMEOUT);
}


void MBReset(void)
{
	mb[0].via[0].Reset();
	mb[0].via[1].Reset();
	mb[0].ay[0].Reset();
	mb[0].ay[1].Reset();
}


void MBWrite(int chipNum, uint8_t reg, uint8_t byte)
{
	NoteMBActivity();
	V6522VIA * chip1 = &mb[0].via[chipNum];
	chip1->Write(reg, byte);

	if (reg == 0)
		mb[0].ay[chipNum].WriteControl(chip1->orb & chip1->ddrb);
	else if (reg == 1)
		mb[0].ay[chipNum].WriteData(chip1->ora & chip1->ddra);
}


uint8_t MBRead(int chipNum, uint8_t reg)
{
	NoteMBActivity();
	return mb[0].via[chipNum].Read(reg);
}


void MBRun(uint16_t cycles)
{
	if (mb[0].via[0].Run(cycles))
		mainCPU.cpuFlags |= V65C02_ASSERT_LINE_IRQ;

	if (mb[0].via[1].Run(cycles))
		mainCPU.cpuFlags |= V65C02_ASSERT_LINE_NMI;
}


void MBSaveState(FILE * file)
{
	fwrite(&mb[0], 1, sizeof(struct MOCKINGBOARD), file);
	fwrite(&mb[1], 1, sizeof(struct MOCKINGBOARD), file);
}


void MBLoadState(FILE * file)
{
	if (fread(&mb[0], 1, sizeof(struct MOCKINGBOARD), file));
	if (fread(&mb[1], 1, sizeof(struct MOCKINGBOARD), file));
}


static uint8_t SlotPageR(uint16_t address)
{
	uint8_t regNum = address & 0x0F;
	uint8_t chipNum = (address & 0x80) >> 7;

	return MBRead(chipNum, regNum);
}


static void SlotPageW(uint16_t address, uint8_t byte)
{
	uint8_t regNum = address & 0x0F;
	uint8_t chipNum = (address & 0x80) >> 7;

	MBWrite(chipNum, regNum, byte);
}


void InstallMockingboard(uint8_t slot)
{
	SlotData mbDevice = { 0, 0, SlotPageR, SlotPageW, 0, 0 };
	InstallSlotHandler(slot, &mbDevice);
}

