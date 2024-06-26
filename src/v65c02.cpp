//
// Virtual 65C02 Emulator v1.1
//
// by James Hammons
// (c) 2005-2018 Underground Software
//
// JLH = James Hammons <jlhamm@acm.org>
//
// WHO  WHEN        WHAT
// ---  ----------  -----------------------------------------------------------
// JLH  01/04/2006  Added changelog ;-)
// JLH  01/18/2009  Fixed EA_ABS_* macros
//

//OK, the wraparound bug exists in both the Apple and Atari versions of Ultima
//II. However, the Atari version *does* occassionally pick strength while the
//Apple versions do not--which would seem to indicate a bug either in the RNG
//algorithm, the 65C02 core, or the Apple hardware. Need to investigate all
//three!
//N.B.: There were some lingering bugs in the BCD portions of the ADC and SBC
//      opcodes; need to test to see if that clears up the problem.

#define __DEBUG__
//#define __DEBUGMON__

#include "v65c02.h"

#ifdef __DEBUG__
#include <string.h>
#include "dis65c02.h"
#include "floppydrive.h"
#include "log.h"
#endif


// Various helper macros

#define CLR_Z				(regs->cc &= ~FLAG_Z)
#define CLR_ZN				(regs->cc &= ~(FLAG_Z | FLAG_N))
#define CLR_ZNC				(regs->cc &= ~(FLAG_Z | FLAG_N | FLAG_C))
#define CLR_V				(regs->cc &= ~FLAG_V)
#define CLR_N				(regs->cc &= ~FLAG_N)
#define CLR_D				(regs->cc &= ~FLAG_D)
#define SET_Z(r)			(regs->cc = ((r) == 0 ? regs->cc | FLAG_Z : regs->cc & ~FLAG_Z))
#define SET_N(r)			(regs->cc = ((r) & 0x80 ? regs->cc | FLAG_N : regs->cc & ~FLAG_N))
#define SET_I				(regs->cc |= FLAG_I)

//Not sure that this code is computing the carry correctly... Investigate! [Seems to be]
/*
Not 100% sure (for SET_C_CMP), when we have things like this:
D0BE: AC 6F D3  LDY  $D36F     [SP=01EC, CC=--.--IZ-, A=AA, X=60, Y=00]
D0C1: CC 5A D3  CPY  $D35A     [SP=01EC, CC=--.--IZC, A=AA, X=60, Y=00]
D0C4: F0 0F     BEQ  $D0D5     [SP=01EC, CC=--.--IZC, A=AA, X=60, Y=00]
D0D5: AD 6E D3  LDA  $D36E     [SP=01EC, CC=--.--I-C, A=0A, X=60, Y=00]

Which shows that $D35A has to be 0 since the Z flag is set.  Why would the carry flag be set on a comparison where the compared items are equal?
*/
#define SET_C_ADD(a,b)		(regs->cc = ((uint8_t)(b) > (uint8_t)(~(a)) ? regs->cc | FLAG_C : regs->cc & ~FLAG_C))
#define SET_C_CMP(a,b)		(regs->cc = ((uint8_t)(b) >= (uint8_t)(a) ? regs->cc | FLAG_C : regs->cc & ~FLAG_C))
#define SET_ZN(r)			SET_N(r); SET_Z(r)
#define SET_ZNC_ADD(a,b,r)	SET_N(r); SET_Z(r); SET_C_ADD(a,b)
#define SET_ZNC_CMP(a,b,r)	SET_N(r); SET_Z(r); SET_C_CMP(a,b)

#define EA_IMM				regs->pc++
#define EA_ZP				regs->RdMem(regs->pc++)
#define EA_ZP_X				(regs->RdMem(regs->pc++) + regs->x) & 0xFF
#define EA_ZP_Y				(regs->RdMem(regs->pc++) + regs->y) & 0xFF
#define EA_ABS				FetchMemW(regs->pc)
#define EA_ABS_X			FetchMemW(regs->pc) + regs->x
#define EA_ABS_Y			FetchMemW(regs->pc) + regs->y
#define EA_IND_ZP_X			RdMemWZP((regs->RdMem(regs->pc++) + regs->x) & 0xFF)
#define EA_IND_ZP_Y			RdMemWZP(regs->RdMem(regs->pc++)) + regs->y
#define EA_IND_ZP			RdMemWZP(regs->RdMem(regs->pc++))

#define READ_IMM			regs->RdMem(EA_IMM)
#define READ_ZP				regs->RdMem(EA_ZP)
#define READ_ZP_X			regs->RdMem(EA_ZP_X)
#define READ_ZP_Y			regs->RdMem(EA_ZP_Y)
#define READ_ABS			regs->RdMem(EA_ABS)
#define READ_ABS_X			regs->RdMem(EA_ABS_X)
#define READ_ABS_Y			regs->RdMem(EA_ABS_Y)
#define READ_IND_ZP_X		regs->RdMem(EA_IND_ZP_X)
#define READ_IND_ZP_Y		regs->RdMem(EA_IND_ZP_Y)
#define READ_IND_ZP			regs->RdMem(EA_IND_ZP)

#define READ_IMM_WB(v)		uint16_t addr = EA_IMM;      v = regs->RdMem(addr)
#define READ_ZP_WB(v)		uint16_t addr = EA_ZP;       v = regs->RdMem(addr)
#define READ_ZP_X_WB(v)		uint16_t addr = EA_ZP_X;     v = regs->RdMem(addr)
#define READ_ABS_WB(v)		uint16_t addr = EA_ABS;      v = regs->RdMem(addr)
#define READ_ABS_X_WB(v)	uint16_t addr = EA_ABS_X;    v = regs->RdMem(addr)
#define READ_ABS_Y_WB(v)	uint16_t addr = EA_ABS_Y;    v = regs->RdMem(addr)
#define READ_IND_ZP_X_WB(v)	uint16_t addr = EA_IND_ZP_X; v = regs->RdMem(addr)
#define READ_IND_ZP_Y_WB(v)	uint16_t addr = EA_IND_ZP_Y; v = regs->RdMem(addr)
#define READ_IND_ZP_WB(v)	uint16_t addr = EA_IND_ZP;   v = regs->RdMem(addr)

#define WRITE_BACK(d)		regs->WrMem(addr, (d))


// Private global variables

static V65C02REGS * regs;

// Cycle counts should be correct for the the Rockwell version of the 65C02.
// Extra cycles for page crossing or BCD mode are accounted for in their
// respective opcode handlers.
static uint8_t CPUCycles[256] = {
	7, 6, 2, 2, 5, 3, 5, 5, 3, 2, 2, 2, 6, 4, 6, 5,
	2, 5, 5, 2, 5, 4, 6, 5, 2, 4, 2, 2, 6, 4, 6, 5,
	6, 6, 2, 2, 3, 3, 5, 5, 4, 2, 2, 2, 4, 2, 6, 5,
	2, 5, 5, 2, 4, 4, 6, 5, 2, 4, 2, 2, 4, 4, 6, 5,
	6, 6, 2, 2, 3, 3, 5, 5, 3, 2, 2, 2, 3, 4, 6, 5,
	2, 5, 5, 2, 4, 4, 6, 5, 2, 4, 3, 2, 8, 4, 6, 5,
	6, 6, 2, 2, 3, 3, 5, 5, 4, 2, 2, 2, 6, 4, 6, 5,
	2, 5, 5, 2, 4, 4, 6, 5, 2, 4, 4, 2, 6, 4, 6, 5,
	2, 6, 2, 2, 3, 3, 3, 5, 2, 2, 2, 2, 4, 4, 4, 5,
	2, 6, 5, 2, 4, 4, 4, 5, 2, 5, 2, 2, 4, 5, 5, 5,
	2, 6, 2, 2, 3, 3, 3, 5, 2, 2, 2, 2, 4, 4, 4, 5,
	2, 5, 5, 2, 4, 4, 4, 5, 2, 4, 2, 2, 4, 4, 4, 5,
	2, 6, 2, 2, 3, 3, 5, 5, 2, 2, 2, 2, 4, 4, 5, 5,
	2, 5, 5, 2, 4, 4, 6, 5, 2, 4, 3, 2, 4, 4, 6, 5,
	2, 6, 2, 2, 3, 3, 5, 5, 2, 2, 2, 2, 4, 4, 6, 5,
	2, 5, 5, 2, 4, 4, 6, 5, 2, 4, 4, 2, 4, 4, 6, 5 };


//
// Read a uint16_t out of 65C02 memory (big endian format)
//
static inline uint16_t RdMemW(uint16_t address)
{
	return (uint16_t)(regs->RdMem(address + 1) << 8)
		| regs->RdMem(address + 0);
}


//
// Read a uint16_t out of 65C02 memory (big endian format), wrapping on page 0
//
static inline uint16_t RdMemWZP(uint16_t address)
{
	return (uint16_t)(regs->RdMem((address + 1) & 0xFF) << 8)
		| regs->RdMem(address + 0);
}


//
// Read a uint16_t out of 65C02 memory (big endian format) and increment PC
//
static inline uint16_t FetchMemW(uint16_t address)
{
	regs->pc += 2;
	return (uint16_t)(regs->RdMem(address + 1) << 8)
		| regs->RdMem(address + 0);
}


//
// 65C02 OPCODE IMPLEMENTATION
//
// NOTE: Lots of macros are used here to save a LOT of typing.  Also
//       helps speed the debugging process.  :-)  Because of this, combining
//       certain lines may look like a good idea but would end in disaster.
//       You have been warned!  ;-)
//

// Page crossing macros.  These catch the cases where access of a certain type
// will incur a one cycle penalty when crossing a page boundary.

#define HANDLE_PAGE_CROSSING_IND_Y \
	uint16_t addressLo = regs->RdMem(regs->RdMem(regs->pc)); \
\
	if ((addressLo + regs->y) > 0xFF) \
		regs->clock++;

#define HANDLE_PAGE_CROSSING_ABS_X \
	uint16_t addressLo = regs->RdMem(regs->pc); \
\
	if ((addressLo + regs->x) > 0xFF) \
		regs->clock++;

#define HANDLE_PAGE_CROSSING_ABS_Y \
	uint16_t addressLo = regs->RdMem(regs->pc); \
\
	if ((addressLo + regs->y) > 0xFF) \
		regs->clock++;

// Branch taken adds a cycle, crossing page adds one more

#define HANDLE_BRANCH_TAKEN(m)       \
{                                    \
	uint16_t oldpc = regs->pc;       \
	regs->pc += m;                   \
	regs->clock++;                   \
                                     \
	if ((oldpc ^ regs->pc) & 0xFF00) \
		regs->clock++;               \
}

/*
Mnemonic	Addressing mode	Form		Opcode	Size	Timing

ADC			Immediate		ADC #Oper	69		2		2
			Zero Page		ADC Zpg		65		2		3
			Zero Page,X		ADC Zpg,X	75		2		4
			Absolute		ADC Abs		6D		3		4
			Absolute,X		ADC Abs,X	7D		3		4
			Absolute,Y		ADC Abs,Y	79		3		4
			(Zero Page,X)	ADC (Zpg,X)	61		2		6
			(Zero Page),Y	ADC (Zpg),Y	71		2		5
			(Zero Page)		ADC (Zpg)	72		2		5
*/

// ADC opcodes

//This is non-optimal, but it works--optimize later. :-)
//N.B.: We have to pull the low nybble from each part of the sum in order to
//      check BCD addition of the low nybble correctly.  It doesn't work to
//      look at the sum after summing the bytes.  Also, Decimal mode incurs a
//      one cycle penalty (for the decimal correction).
#define OP_ADC_HANDLER(m) \
	uint16_t sum = (uint16_t)regs->a + (m) + (uint16_t)(regs->cc & FLAG_C); \
\
	if (regs->cc & FLAG_D) \
	{ \
		uint8_t an = regs->a & 0x0F, mn = (m) & 0x0F, cn = (uint8_t)(regs->cc & FLAG_C); \
\
		if ((an + mn + cn) > 9) \
			sum += 0x06; \
\
		if ((sum & 0x1F0) > 0x90) \
			sum += 0x60; \
\
		regs->clock++;\
	} \
\
	regs->cc = (regs->cc & ~FLAG_C) | (sum >> 8); \
	regs->cc = (~(regs->a ^ (m)) & (regs->a ^ sum) & 0x80 ? regs->cc | FLAG_V : regs->cc & ~FLAG_V); \
	regs->a = sum & 0xFF; \
	SET_ZN(regs->a)

static void Op69(void)							// ADC #
{
	uint16_t m = READ_IMM;
	OP_ADC_HANDLER(m);
}

static void Op65(void)							// ADC ZP
{
	uint16_t m = READ_ZP;
	OP_ADC_HANDLER(m);
}

static void Op75(void)							// ADC ZP, X
{
	uint16_t m = READ_ZP_X;
	OP_ADC_HANDLER(m);
}

static void Op6D(void)							// ADC ABS
{
	uint16_t m = READ_ABS;
	OP_ADC_HANDLER(m);
}

static void Op7D(void)							// ADC ABS, X
{
	HANDLE_PAGE_CROSSING_ABS_X;
	uint16_t m = READ_ABS_X;
	OP_ADC_HANDLER(m);
}

static void Op79(void)							// ADC ABS, Y
{
	HANDLE_PAGE_CROSSING_ABS_Y;
	uint16_t m = READ_ABS_Y;
	OP_ADC_HANDLER(m);
}

static void Op61(void)							// ADC (ZP, X)
{
	uint16_t m = READ_IND_ZP_X;
	OP_ADC_HANDLER(m);
}

static void Op71(void)							// ADC (ZP), Y
{
	HANDLE_PAGE_CROSSING_IND_Y;
	uint16_t m = READ_IND_ZP_Y;
	OP_ADC_HANDLER(m);
}

static void Op72(void)							// ADC (ZP)
{
	uint16_t m = READ_IND_ZP;
	OP_ADC_HANDLER(m);
}

/*
AND	Immediate	AND #Oper	29	2	2
Zero Page		AND Zpg		25	2	3
Zero Page,X		AND Zpg,X	35	2	4
Absolute		AND Abs		2D	3	4
Absolute,X		AND Abs,X	3D	3	4
Absolute,Y		AND Abs,Y	39	3	4
(Zero Page,X)	AND (Zpg,X)	21	2	6
(Zero Page),Y	AND (Zpg),Y	31	2	5
(Zero Page)		AND (Zpg)	32	2	5
*/

// AND opcodes

#define OP_AND_HANDLER(m) \
	regs->a &= m; \
	SET_ZN(regs->a)

static void Op29(void)							// AND #
{
	uint8_t m = READ_IMM;
	OP_AND_HANDLER(m);
}

static void Op25(void)							// AND ZP
{
	uint8_t m = READ_ZP;
	OP_AND_HANDLER(m);
}

static void Op35(void)							// AND ZP, X
{
	uint8_t m = READ_ZP_X;
	OP_AND_HANDLER(m);
}

static void Op2D(void)							// AND ABS
{
	uint8_t m = READ_ABS;
	OP_AND_HANDLER(m);
}

static void Op3D(void)							// AND ABS, X
{
	HANDLE_PAGE_CROSSING_ABS_X;
	uint8_t m = READ_ABS_X;
	OP_AND_HANDLER(m);
}

static void Op39(void)							// AND ABS, Y
{
	HANDLE_PAGE_CROSSING_ABS_Y;
	uint8_t m = READ_ABS_Y;
	OP_AND_HANDLER(m);
}

static void Op21(void)							// AND (ZP, X)
{
	uint8_t m = READ_IND_ZP_X;
	OP_AND_HANDLER(m);
}

static void Op31(void)							// AND (ZP), Y
{
	HANDLE_PAGE_CROSSING_IND_Y;
	uint8_t m = READ_IND_ZP_Y;
	OP_AND_HANDLER(m);
}

static void Op32(void)							// AND (ZP)
{
	uint8_t m = READ_IND_ZP;
	OP_AND_HANDLER(m);
}

/*
ASL	Accumulator	ASL A		0A	1	2
Zero Page		ASL Zpg		06	2	5
Zero Page,X		ASL Zpg,X	16	2	6
Absolute		ASL Abs		0E	3	6
Absolute,X		ASL Abs,X	1E	3	7
*/

// ASL opcodes

#define OP_ASL_HANDLER(m) \
	regs->cc = ((m) & 0x80 ? regs->cc | FLAG_C : regs->cc & ~FLAG_C); \
	(m) <<= 1; \
	SET_ZN((m))

static void Op0A(void)							// ASL A
{
	OP_ASL_HANDLER(regs->a);
}

static void Op06(void)							// ASL ZP
{
	uint8_t m;
	READ_ZP_WB(m);
	OP_ASL_HANDLER(m);
	WRITE_BACK(m);
}

static void Op16(void)							// ASL ZP, X
{
	uint8_t m;
	READ_ZP_X_WB(m);
	OP_ASL_HANDLER(m);
	WRITE_BACK(m);
}

static void Op0E(void)							// ASL ABS
{
	uint8_t m;
	READ_ABS_WB(m);
	OP_ASL_HANDLER(m);
	WRITE_BACK(m);
}

static void Op1E(void)							// ASL ABS, X
{
	HANDLE_PAGE_CROSSING_ABS_X;
	uint8_t m;
	READ_ABS_X_WB(m);
	OP_ASL_HANDLER(m);
	WRITE_BACK(m);
}

/*
BBR0	ZP, Relative	BBR0 Oper	0F	3	5
BBR1	ZP, Relative	BBR1 Oper	1F	3	5
BBR2	ZP, Relative	BBR2 Oper	2F	3	5
BBR3	ZP, Relative	BBR3 Oper	3F	3	5
BBR4	ZP, Relative	BBR4 Oper	4F	3	5
BBR5	ZP, Relative	BBR5 Oper	5F	3	5
BBR6	ZP, Relative	BBR6 Oper	6F	3	5
BBR7	ZP, Relative	BBR7 Oper	7F	3	5
BBS0	ZP, Relative	BBS0 Oper	8F	3	5
BBS1	ZP, Relative	BBS1 Oper	9F	3	5
BBS2	ZP, Relative	BBS2 Oper	AF	3	5
BBS3	ZP, Relative	BBS3 Oper	BF	3	5
BBS4	ZP, Relative	BBS4 Oper	CF	3	5
BBS5	ZP, Relative	BBS5 Oper	DF	3	5
BBS6	ZP, Relative	BBS6 Oper	EF	3	5
BBS7	ZP, Relative	BBS7 Oper	FF	3	5
*/

// BBR/Sn opcodes

static void Op0F(void)							// BBR0
{
	uint8_t b = READ_ZP;
	int16_t m = (int16_t)(int8_t)READ_IMM;

	if (!(b & 0x01))
		HANDLE_BRANCH_TAKEN(m);
}

static void Op1F(void)							// BBR1
{
	uint8_t b = READ_ZP;
	int16_t m = (int16_t)(int8_t)READ_IMM;

	if (!(b & 0x02))
		HANDLE_BRANCH_TAKEN(m);
}

static void Op2F(void)							// BBR2
{
	uint8_t b = READ_ZP;
	int16_t m = (int16_t)(int8_t)READ_IMM;

	if (!(b & 0x04))
		HANDLE_BRANCH_TAKEN(m);
}

static void Op3F(void)							// BBR3
{
	uint8_t b = READ_ZP;
	int16_t m = (int16_t)(int8_t)READ_IMM;

	if (!(b & 0x08))
		HANDLE_BRANCH_TAKEN(m);
}

static void Op4F(void)							// BBR4
{
	uint8_t b = READ_ZP;
	int16_t m = (int16_t)(int8_t)READ_IMM;

	if (!(b & 0x10))
		HANDLE_BRANCH_TAKEN(m);
}

static void Op5F(void)							// BBR5
{
	uint8_t b = READ_ZP;
	int16_t m = (int16_t)(int8_t)READ_IMM;

	if (!(b & 0x20))
		HANDLE_BRANCH_TAKEN(m);
}

static void Op6F(void)							// BBR6
{
	uint8_t b = READ_ZP;
	int16_t m = (int16_t)(int8_t)READ_IMM;

	if (!(b & 0x40))
		HANDLE_BRANCH_TAKEN(m);
}

static void Op7F(void)							// BBR7
{
	uint8_t b = READ_ZP;
	int16_t m = (int16_t)(int8_t)READ_IMM;

	if (!(b & 0x80))
		HANDLE_BRANCH_TAKEN(m);
}

static void Op8F(void)							// BBS0
{
	uint8_t b = READ_ZP;
	int16_t m = (int16_t)(int8_t)READ_IMM;

	if (b & 0x01)
		HANDLE_BRANCH_TAKEN(m);
}

static void Op9F(void)							// BBS1
{
	uint8_t b = READ_ZP;
	int16_t m = (int16_t)(int8_t)READ_IMM;

	if (b & 0x02)
		HANDLE_BRANCH_TAKEN(m);
}

static void OpAF(void)							// BBS2
{
	uint8_t b = READ_ZP;
	int16_t m = (int16_t)(int8_t)READ_IMM;

	if (b & 0x04)
		HANDLE_BRANCH_TAKEN(m);
}

static void OpBF(void)							// BBS3
{
	uint8_t b = READ_ZP;
	int16_t m = (int16_t)(int8_t)READ_IMM;

	if (b & 0x08)
		HANDLE_BRANCH_TAKEN(m);
}

static void OpCF(void)							// BBS4
{
	uint8_t b = READ_ZP;
	int16_t m = (int16_t)(int8_t)READ_IMM;

	if (b & 0x10)
		HANDLE_BRANCH_TAKEN(m);
}

static void OpDF(void)							// BBS5
{
	uint8_t b = READ_ZP;
	int16_t m = (int16_t)(int8_t)READ_IMM;

	if (b & 0x20)
		HANDLE_BRANCH_TAKEN(m);
}

static void OpEF(void)							// BBS6
{
	uint8_t b = READ_ZP;
	int16_t m = (int16_t)(int8_t)READ_IMM;

	if (b & 0x40)
		HANDLE_BRANCH_TAKEN(m);
}

static void OpFF(void)							// BBS7
{
	uint8_t b = READ_ZP;
	int16_t m = (int16_t)(int8_t)READ_IMM;

	if (b & 0x80)
		HANDLE_BRANCH_TAKEN(m);
}

/*
BCC	Relative	BCC Oper	90	2	2
BCS	Relative	BCS Oper	B0	2	2
BEQ	Relative	BEQ Oper	F0	2	2
*/

// Branch opcodes

static void Op90(void)							// BCC
{
	int16_t m = (int16_t)(int8_t)READ_IMM;

	if (!(regs->cc & FLAG_C))
		HANDLE_BRANCH_TAKEN(m)
}

static void OpB0(void)							// BCS
{
	int16_t m = (int16_t)(int8_t)READ_IMM;

	if (regs->cc & FLAG_C)
		HANDLE_BRANCH_TAKEN(m)
}

static void OpF0(void)							// BEQ
{
	int16_t m = (int16_t)(int8_t)READ_IMM;

	if (regs->cc & FLAG_Z)
		HANDLE_BRANCH_TAKEN(m)
}

/*
BIT	Immediate	BIT #Oper	89	2	2
Zero Page		BIT Zpg		24	2	3
Zero Page,X		BIT Zpg,X	34	2	4
Absolute		BIT Abs		2C	3	4
Absolute,X		BIT Abs,X	3C	3	4
*/

// BIT opcodes

/* 1. The BIT instruction copies bit 6 to the V flag, and bit 7 to the N flag
      (except in immediate addressing mode where V & N are untouched.) The
      accumulator and the operand are ANDed and the Z flag is set
      appropriately. */

#define OP_BIT_HANDLER(m) \
	int8_t result = regs->a & (m); \
	regs->cc &= ~(FLAG_N | FLAG_V); \
	regs->cc |= ((m) & 0xC0); \
	SET_Z(result)

static void Op89(void)							// BIT #
{
	int8_t m = READ_IMM;
	int8_t result = regs->a & m;
	SET_Z(result);
}

static void Op24(void)							// BIT ZP
{
	int8_t m = READ_ZP;
	OP_BIT_HANDLER(m);
}

static void Op34(void)							// BIT ZP, X
{
	uint8_t m = READ_ZP_X;
	OP_BIT_HANDLER(m);
}

static void Op2C(void)							// BIT ABS
{
	uint8_t m = READ_ABS;
	OP_BIT_HANDLER(m);
}

static void Op3C(void)							// BIT ABS, X
{
	HANDLE_PAGE_CROSSING_ABS_X;
	uint8_t m = READ_ABS_X;
	OP_BIT_HANDLER(m);
}

/*
BMI	Relative	BMI Oper	30	2	2
BNE	Relative	BNE Oper	D0	2	2
BPL	Relative	BPL Oper	10	2	2
BRA	Relative	BRA Oper	80	2	3
*/

// More branch opcodes

static void Op30(void)							// BMI
{
	int16_t m = (int16_t)(int8_t)READ_IMM;

	if (regs->cc & FLAG_N)
		HANDLE_BRANCH_TAKEN(m)
}

static void OpD0(void)							// BNE
{
	int16_t m = (int16_t)(int8_t)READ_IMM;

	if (!(regs->cc & FLAG_Z))
		HANDLE_BRANCH_TAKEN(m)
}

static void Op10(void)							// BPL
{
	int16_t m = (int16_t)(int8_t)READ_IMM;

	if (!(regs->cc & FLAG_N))
		HANDLE_BRANCH_TAKEN(m)
}

static void Op80(void)							// BRA
{
	int16_t m = (int16_t)(int8_t)READ_IMM;
	HANDLE_BRANCH_TAKEN(m)
}

/*
BRK	Implied		BRK			00	1	7
*/

static void Op00(void)							// BRK
{
//#ifdef __DEBUG__
#if 1
WriteLog("\n*** BRK ***\n\n");
WriteLog(" [PC=%04X, SP=%04X, CC=%s%s.%s%s%s%s%s, A=%02X, X=%02X, Y=%02X]\n",
	regs->pc, 0x0100 + regs->sp,
	(regs->cc & FLAG_N ? "N" : "-"), (regs->cc & FLAG_V ? "V" : "-"),
	(regs->cc & FLAG_B ? "B" : "-"), (regs->cc & FLAG_D ? "D" : "-"),
	(regs->cc & FLAG_I ? "I" : "-"), (regs->cc & FLAG_Z ? "Z" : "-"),
	(regs->cc & FLAG_C ? "C" : "-"), regs->a, regs->x, regs->y);
#endif
	regs->cc |= FLAG_B;							// Set B
	regs->pc++;									// RTI comes back to the instruction one byte after the BRK
	regs->WrMem(0x0100 + regs->sp--, regs->pc >> 8);	// Save PC and CC
	regs->WrMem(0x0100 + regs->sp--, regs->pc & 0xFF);
	regs->WrMem(0x0100 + regs->sp--, regs->cc);
	regs->cc |= FLAG_I;							// Set I
	regs->cc &= ~FLAG_D;							// & clear D
	regs->pc = RdMemW(0xFFFE);					// Grab the IRQ vector & go...
}

/*
BVC	Relative	BVC Oper	50	2	2
BVS	Relative	BVS Oper	70	2	2
*/

// Even more branch opcodes

static void Op50(void)							// BVC
{
	int16_t m = (int16_t)(int8_t)READ_IMM;

	if (!(regs->cc & FLAG_V))
		HANDLE_BRANCH_TAKEN(m)
}

static void Op70(void)							// BVS
{
	int16_t m = (int16_t)(int8_t)READ_IMM;

	if (regs->cc & FLAG_V)
		HANDLE_BRANCH_TAKEN(m)
}

/*
CLC	Implied		CLC			18	1	2
*/

static void Op18(void)							// CLC
{
	regs->cc &= ~FLAG_C;
}

/*
CLD	Implied		CLD			D8	1	2
*/

static void OpD8(void)							// CLD
{
	CLR_D;
}

/*
CLI	Implied		CLI			58	1	2
*/

static void Op58(void)							// CLI
{
	regs->cc &= ~FLAG_I;
}

/*
CLV	Implied		CLV			B8	1	2
*/

static void OpB8(void)							// CLV
{
	regs->cc &= ~FLAG_V;
}

/*
CMP	Immediate	CMP #Oper	C9	2	2
Zero Page		CMP Zpg		C5	2	3
Zero Page,X		CMP Zpg		D5	2	4
Absolute		CMP Abs		CD	3	4
Absolute,X		CMP Abs,X	DD	3	4
Absolute,Y		CMP Abs,Y	D9	3	4
(Zero Page,X)	CMP (Zpg,X)	C1	2	6
(Zero Page),Y	CMP (Zpg),Y	D1	2	5
(Zero Page)		CMP (Zpg)	D2	2	5
*/

// CMP opcodes

#define OP_CMP_HANDLER(m) \
	uint8_t result = regs->a - (m); \
	SET_ZNC_CMP(m, regs->a, result)

static void OpC9(void)							// CMP #
{
	uint8_t m = READ_IMM;
	OP_CMP_HANDLER(m);
}

static void OpC5(void)							// CMP ZP
{
	uint8_t m = READ_ZP;
	OP_CMP_HANDLER(m);
}

static void OpD5(void)							// CMP ZP, X
{
	uint8_t m = READ_ZP_X;
	OP_CMP_HANDLER(m);
}

static void OpCD(void)							// CMP ABS
{
	uint8_t m = READ_ABS;
	OP_CMP_HANDLER(m);
}

static void OpDD(void)							// CMP ABS, X
{
	HANDLE_PAGE_CROSSING_ABS_X;
	uint8_t m = READ_ABS_X;
	OP_CMP_HANDLER(m);
}

static void OpD9(void)							// CMP ABS, Y
{
	HANDLE_PAGE_CROSSING_ABS_Y;
	uint8_t m = READ_ABS_Y;
	OP_CMP_HANDLER(m);
}

static void OpC1(void)							// CMP (ZP, X)
{
	uint8_t m = READ_IND_ZP_X;
	OP_CMP_HANDLER(m);
}

static void OpD1(void)							// CMP (ZP), Y
{
	HANDLE_PAGE_CROSSING_IND_Y;
	uint8_t m = READ_IND_ZP_Y;
	OP_CMP_HANDLER(m);
}

static void OpD2(void)							// CMP (ZP)
{
	uint8_t m = READ_IND_ZP;
	OP_CMP_HANDLER(m);
}

/*
CPX	Immediate	CPX #Oper	E0	2	2
Zero Page		CPX Zpg		E4	2	3
Absolute		CPX Abs		EC	3	4
*/

// CPX opcodes

#define OP_CPX_HANDLER(m) \
	uint8_t result = regs->x - (m); \
	SET_ZNC_CMP(m, regs->x, result)

static void OpE0(void)							// CPX #
{
	uint8_t m = READ_IMM;
	OP_CPX_HANDLER(m);
}

static void OpE4(void)							// CPX ZP
{
	uint8_t m = READ_ZP;
	OP_CPX_HANDLER(m);
}

static void OpEC(void)							// CPX ABS
{
	uint8_t m = READ_ABS;
	OP_CPX_HANDLER(m);
}

/*
CPY	Immediate	CPY #Oper	C0	2	2
Zero Page		CPY Zpg		C4	2	3
Absolute		CPY Abs		CC	3	4
*/

// CPY opcodes

#define OP_CPY_HANDLER(m) \
	uint8_t result = regs->y - (m); \
	SET_ZNC_CMP(m, regs->y, result)

static void OpC0(void)							// CPY #
{
	uint8_t m = READ_IMM;
	OP_CPY_HANDLER(m);
}

static void OpC4(void)							// CPY ZP
{
	uint8_t m = READ_ZP;
	OP_CPY_HANDLER(m);
}

static void OpCC(void)							// CPY ABS
{
	uint8_t m = READ_ABS;
	OP_CPY_HANDLER(m);
}

/*
DEA	Accumulator	DEA			3A	1	2
*/

static void Op3A(void)							// DEA
{
	regs->a--;
	SET_ZN(regs->a);
}

/*
DEC	Zero Page	DEC Zpg		C6	2	5
Zero Page,X		DEC Zpg,X	D6	2	6
Absolute		DEC Abs		CE	3	6
Absolute,X		DEC Abs,X	DE	3	7
*/

// DEC opcodes

#define OP_DEC_HANDLER(m) \
	m--; \
	SET_ZN(m)

static void OpC6(void)							// DEC ZP
{
	uint8_t m;
	READ_ZP_WB(m);
	OP_DEC_HANDLER(m);
	WRITE_BACK(m);
}

static void OpD6(void)							// DEC ZP, X
{
	uint8_t m;
	READ_ZP_X_WB(m);
	OP_DEC_HANDLER(m);
	WRITE_BACK(m);
}

static void OpCE(void)							// DEC ABS
{
	uint8_t m;
	READ_ABS_WB(m);
	OP_DEC_HANDLER(m);
	WRITE_BACK(m);
}

static void OpDE(void)							// DEC ABS, X
{
	HANDLE_PAGE_CROSSING_ABS_X;
	uint8_t m;
	READ_ABS_X_WB(m);
	OP_DEC_HANDLER(m);
	WRITE_BACK(m);
}

/*
DEX	Implied		DEX			CA	1	2
*/

static void OpCA(void)							// DEX
{
	regs->x--;
	SET_ZN(regs->x);
}

/*
DEY	Implied		DEY			88	1	2
*/

static void Op88(void)							// DEY
{
	regs->y--;
	SET_ZN(regs->y);
}

/*
EOR	Immediate	EOR #Oper	49	2	2
Zero Page		EOR Zpg		45	2	3
Zero Page,X		EOR Zpg,X	55	2	4
Absolute		EOR Abs		4D	3	4
Absolute,X		EOR Abs,X	5D	3	4
Absolute,Y		EOR Abs,Y	59	3	4
(Zero Page,X)	EOR (Zpg,X)	41	2	6
(Zero Page),Y	EOR (Zpg),Y	51	2	5
(Zero Page)		EOR (Zpg)	52	2	5
*/

// EOR opcodes

#define OP_EOR_HANDLER(m) \
	regs->a ^= m; \
	SET_ZN(regs->a)

static void Op49(void)							// EOR #
{
	uint8_t m = READ_IMM;
	OP_EOR_HANDLER(m);
}

static void Op45(void)							// EOR ZP
{
	uint8_t m = READ_ZP;
	OP_EOR_HANDLER(m);
}

static void Op55(void)							// EOR ZP, X
{
	uint8_t m = READ_ZP_X;
	OP_EOR_HANDLER(m);
}

static void Op4D(void)							// EOR ABS
{
	uint8_t m = READ_ABS;
	OP_EOR_HANDLER(m);
}

static void Op5D(void)							// EOR ABS, X
{
	HANDLE_PAGE_CROSSING_ABS_X;
	uint8_t m = READ_ABS_X;
	OP_EOR_HANDLER(m);
}

static void Op59(void)							// EOR ABS, Y
{
	HANDLE_PAGE_CROSSING_ABS_Y;
	uint8_t m = READ_ABS_Y;
	OP_EOR_HANDLER(m);
}

static void Op41(void)							// EOR (ZP, X)
{
	uint8_t m = READ_IND_ZP_X;
	OP_EOR_HANDLER(m);
}

static void Op51(void)							// EOR (ZP), Y
{
	HANDLE_PAGE_CROSSING_IND_Y;
	uint8_t m = READ_IND_ZP_Y;
	OP_EOR_HANDLER(m);
}

static void Op52(void)							// EOR (ZP)
{
	uint8_t m = READ_IND_ZP;
	OP_EOR_HANDLER(m);
}

/*
INA	Accumulator	INA			1A	1	2
*/

static void Op1A(void)							// INA
{
	regs->a++;
	SET_ZN(regs->a);
}

/*
INC	Zero Page	INC Zpg		E6	2	5
Zero Page,X		INC Zpg,X	F6	2	6
Absolute		INC Abs		EE	3	6
Absolute,X		INC Abs,X	FE	3	7
*/

// INC opcodes

#define OP_INC_HANDLER(m) \
	m++; \
	SET_ZN(m)

static void OpE6(void)							// INC ZP
{
	uint8_t m;
	READ_ZP_WB(m);
	OP_INC_HANDLER(m);
	WRITE_BACK(m);
}

static void OpF6(void)							// INC ZP, X
{
	uint8_t m;
	READ_ZP_X_WB(m);
	OP_INC_HANDLER(m);
	WRITE_BACK(m);
}

static void OpEE(void)							// INC ABS
{
	uint8_t m;
	READ_ABS_WB(m);
	OP_INC_HANDLER(m);
	WRITE_BACK(m);
}

static void OpFE(void)							// INC ABS, X
{
	HANDLE_PAGE_CROSSING_ABS_X;
	uint8_t m;
	READ_ABS_X_WB(m);
	OP_INC_HANDLER(m);
	WRITE_BACK(m);
}

/*
INX	Implied		INX			E8	1	2
*/

static void OpE8(void)							// INX
{
	regs->x++;
	SET_ZN(regs->x);
}

/*
INY	Implied		INY			C8	1	2
*/

static void OpC8(void)							// INY
{
	regs->y++;
	SET_ZN(regs->y);
}

/*
JMP	Absolute	JMP Abs		4C	3	3
(Absolute)		JMP (Abs)	6C	3	5
(Absolute,X)	JMP (Abs,X)	7C	3	6
*/

// JMP opcodes

static void Op4C(void)							// JMP ABS
{
	regs->pc = RdMemW(regs->pc);
}

static void Op6C(void)							// JMP (ABS)
{
	// Check for page crossing
	uint16_t addressLo = regs->RdMem(regs->pc);

	if (addressLo == 0xFF)
		regs->clock++;

	regs->pc = RdMemW(RdMemW(regs->pc));
}

static void Op7C(void)							// JMP (ABS, X)
{
	regs->pc = RdMemW(RdMemW(regs->pc) + regs->x);
}

/*
JSR	Absolute	JSR Abs		20	3	6
*/

static void Op20(void)							// JSR
{
	uint16_t addr = RdMemW(regs->pc);
	regs->pc++;									// Since it pushes return address - 1...
	regs->WrMem(0x0100 + regs->sp--, regs->pc >> 8);
	regs->WrMem(0x0100 + regs->sp--, regs->pc & 0xFF);
	regs->pc = addr;
}

/*
LDA	Immediate	LDA #Oper	A9	2	2
Zero Page		LDA Zpg		A5	2	3
Zero Page,X		LDA Zpg,X	B5	2	4
Absolute		LDA Abs		AD	3	4
Absolute,X		LDA Abs,X	BD	3	4
Absolute,Y		LDA Abs,Y	B9	3	4
(Zero Page,X)	LDA (Zpg,X)	A1	2	6
(Zero Page),Y	LDA (Zpg),Y	B1	2	5
(Zero Page)		LDA (Zpg)	B2	2	5
*/

// LDA opcodes

#define OP_LDA_HANDLER(m) \
	regs->a = m; \
	SET_ZN(regs->a)

static void OpA9(void)							// LDA #
{
	uint8_t m = READ_IMM;
	OP_LDA_HANDLER(m);
}

static void OpA5(void)							// LDA ZP
{
	uint8_t m = READ_ZP;
	OP_LDA_HANDLER(m);
}

static void OpB5(void)							// LDA ZP, X
{
	uint8_t m = READ_ZP_X;
	OP_LDA_HANDLER(m);
}

static void OpAD(void)							// LDA ABS
{
	uint8_t m = READ_ABS;
	OP_LDA_HANDLER(m);
}

static void OpBD(void)							// LDA ABS, X
{
	HANDLE_PAGE_CROSSING_ABS_X;
	uint8_t m = READ_ABS_X;
	OP_LDA_HANDLER(m);
}

static void OpB9(void)							// LDA ABS, Y
{
	HANDLE_PAGE_CROSSING_ABS_Y;
	uint8_t m = READ_ABS_Y;
	OP_LDA_HANDLER(m);
}

static void OpA1(void)							// LDA (ZP, X)
{
	uint8_t m = READ_IND_ZP_X;
	OP_LDA_HANDLER(m);
}

static void OpB1(void)							// LDA (ZP), Y
{
	HANDLE_PAGE_CROSSING_IND_Y;
	uint8_t m = READ_IND_ZP_Y;
	OP_LDA_HANDLER(m);
}

static void OpB2(void)							// LDA (ZP)
{
	uint8_t m = READ_IND_ZP;
	OP_LDA_HANDLER(m);
}

/*
LDX	Immediate	LDX #Oper	A2	2	2
Zero Page		LDX Zpg		A6	2	3
Zero Page,Y		LDX Zpg,Y	B6	2	4
Absolute		LDX Abs		AE	3	4
Absolute,Y		LDX Abs,Y	BE	3	4
*/

// LDX opcodes

#define OP_LDX_HANDLER(m) \
	regs->x = m; \
	SET_ZN(regs->x)

static void OpA2(void)							// LDX #
{
	uint8_t m = READ_IMM;
	OP_LDX_HANDLER(m);
}

static void OpA6(void)							// LDX ZP
{
	uint8_t m = READ_ZP;
	OP_LDX_HANDLER(m);
}

static void OpB6(void)							// LDX ZP, Y
{
	uint8_t m = READ_ZP_Y;
	OP_LDX_HANDLER(m);
}

static void OpAE(void)							// LDX ABS
{
	uint8_t m = READ_ABS;
	OP_LDX_HANDLER(m);
}

static void OpBE(void)							// LDX ABS, Y
{
	HANDLE_PAGE_CROSSING_ABS_Y;
	uint8_t m = READ_ABS_Y;
	OP_LDX_HANDLER(m);
}

/*
LDY	Immediate	LDY #Oper	A0	2	2
Zero Page		LDY Zpg		A4	2	3
Zero Page,X		LDY Zpg,X	B4	2	4
Absolute		LDY Abs		AC	3	4
Absolute,X		LDY Abs,X	BC	3	4
*/

// LDY opcodes

#define OP_LDY_HANDLER(m) \
	regs->y = m; \
	SET_ZN(regs->y)

static void OpA0(void)							// LDY #
{
	uint8_t m = READ_IMM;
	OP_LDY_HANDLER(m);
}

static void OpA4(void)							// LDY ZP
{
	uint8_t m = READ_ZP;
	OP_LDY_HANDLER(m);
}

static void OpB4(void)							// LDY ZP, X
{
	uint8_t m = READ_ZP_X;
	OP_LDY_HANDLER(m);
}

static void OpAC(void)							// LDY ABS
{
	uint8_t m = READ_ABS;
	OP_LDY_HANDLER(m);
}

static void OpBC(void)							// LDY ABS, X
{
	HANDLE_PAGE_CROSSING_ABS_X;
	uint8_t m = READ_ABS_X;
	OP_LDY_HANDLER(m);
}

/*
LSR	Accumulator	LSR A		4A	1	2
Zero Page		LSR Zpg		46	2	5
Zero Page,X		LSR Zpg,X	56	2	6
Absolute		LSR Abs		4E	3	6
Absolute,X		LSR Abs,X	5E	3	7
*/

// LSR opcodes

#define OP_LSR_HANDLER(m) \
	regs->cc = ((m) & 0x01 ? regs->cc | FLAG_C : regs->cc & ~FLAG_C); \
	(m) >>= 1; \
	CLR_N; SET_Z((m))

static void Op4A(void)							// LSR A
{
	OP_LSR_HANDLER(regs->a);
}

static void Op46(void)							// LSR ZP
{
	uint8_t m;
	READ_ZP_WB(m);
	OP_LSR_HANDLER(m);
	WRITE_BACK(m);
}

static void Op56(void)							// LSR ZP, X
{
	uint8_t m;
	READ_ZP_X_WB(m);
	OP_LSR_HANDLER(m);
	WRITE_BACK(m);
}

static void Op4E(void)							// LSR ABS
{
	uint8_t m;
	READ_ABS_WB(m);
	OP_LSR_HANDLER(m);
	WRITE_BACK(m);
}

static void Op5E(void)							// LSR ABS, X
{
	HANDLE_PAGE_CROSSING_ABS_X;
	uint8_t m;
	READ_ABS_X_WB(m);
	OP_LSR_HANDLER(m);
	WRITE_BACK(m);
}

/*
NOP	Implied		NOP			EA	1	2
*/

static void OpEA(void)							// NOP
{
}

/*
ORA	Immediate	ORA #Oper	09	2	2
Zero Page		ORA Zpg		05	2	3
Zero Page,X		ORA Zpg,X	15	2	4
Absolute		ORA Abs		0D	3	4
Absolute,X		ORA Abs,X	1D	3	4
Absolute,Y		ORA Abs,Y	19	3	4
(Zero Page,X)	ORA (Zpg,X)	01	2	6
(Zero Page),Y	ORA (Zpg),Y	11	2	5
(Zero Page)		ORA (Zpg)	12	2	5
*/

// ORA opcodes

#define OP_ORA_HANDLER(m) \
	regs->a |= m; \
	SET_ZN(regs->a)

static void Op09(void)							// ORA #
{
	uint8_t m = READ_IMM;
	OP_ORA_HANDLER(m);
}

static void Op05(void)							// ORA ZP
{
	uint8_t m = READ_ZP;
	OP_ORA_HANDLER(m);
}

static void Op15(void)							// ORA ZP, X
{
	uint8_t m = READ_ZP_X;
	OP_ORA_HANDLER(m);
}

static void Op0D(void)							// ORA ABS
{
	uint8_t m = READ_ABS;
	OP_ORA_HANDLER(m);
}

static void Op1D(void)							// ORA ABS, X
{
	HANDLE_PAGE_CROSSING_ABS_X;
	uint8_t m = READ_ABS_X;
	OP_ORA_HANDLER(m);
}

static void Op19(void)							// ORA ABS, Y
{
	HANDLE_PAGE_CROSSING_ABS_Y;
	uint8_t m = READ_ABS_Y;
	OP_ORA_HANDLER(m);
}

static void Op01(void)							// ORA (ZP, X)
{
	uint8_t m = READ_IND_ZP_X;
	OP_ORA_HANDLER(m);
}

static void Op11(void)							// ORA (ZP), Y
{
	HANDLE_PAGE_CROSSING_IND_Y;
	uint8_t m = READ_IND_ZP_Y;
	OP_ORA_HANDLER(m);
}

static void Op12(void)							// ORA (ZP)
{
	uint8_t m = READ_IND_ZP;
	OP_ORA_HANDLER(m);
}

/*
PHA	Implied		PHA			48	1	3
*/

static void Op48(void)							// PHA
{
	regs->WrMem(0x0100 + regs->sp--, regs->a);
}

static void Op08(void)							// PHP
{
	regs->cc |= FLAG_UNK;						// Make sure that the unused bit is always set
	regs->WrMem(0x0100 + regs->sp--, regs->cc);
}

/*
PHX	Implied		PHX			DA	1	3
*/

static void OpDA(void)							// PHX
{
	regs->WrMem(0x0100 + regs->sp--, regs->x);
}

/*
PHY	Implied		PHY			5A	1	3
*/

static void Op5A(void)							// PHY
{
	regs->WrMem(0x0100 + regs->sp--, regs->y);
}

/*
PLA	Implied		PLA			68	1	4
*/

static void Op68(void)							// PLA
{
	regs->a = regs->RdMem(0x0100 + ++regs->sp);
	SET_ZN(regs->a);
}

static void Op28(void)							// PLP
{
	regs->cc = regs->RdMem(0x0100 + ++regs->sp);
}

/*
PLX	Implied		PLX			FA	1	4
*/

static void OpFA(void)							// PLX
{
	regs->x = regs->RdMem(0x0100 + ++regs->sp);
	SET_ZN(regs->x);
}

/*
PLY	Implied		PLY			7A	1	4
*/

static void Op7A(void)							// PLY
{
	regs->y = regs->RdMem(0x0100 + ++regs->sp);
	SET_ZN(regs->y);
}

/*
The bit set and clear instructions have the form xyyy0111, where x is 0 to clear a bit or 1 to set it, and yyy is which bit at the memory location to set or clear.
   RMB0  RMB1  RMB2  RMB3  RMB4  RMB5  RMB6  RMB7
  zp  07  17  27  37  47  57  67  77
     SMB0  SMB1  SMB2  SMB3  SMB4  SMB5  SMB6  SMB7
  zp  87  97  A7  B7  C7  D7  E7  F7
*/

// RMB opcodes

static void Op07(void)							// RMB0 ZP
{
	uint8_t m;
	READ_ZP_WB(m);
	m &= 0xFE;
	WRITE_BACK(m);
}

static void Op17(void)							// RMB1 ZP
{
	uint8_t m;
	READ_ZP_WB(m);
	m &= 0xFD;
	WRITE_BACK(m);
}

static void Op27(void)							// RMB2 ZP
{
	uint8_t m;
	READ_ZP_WB(m);
	m &= 0xFB;
	WRITE_BACK(m);
}

static void Op37(void)							// RMB3 ZP
{
	uint8_t m;
	READ_ZP_WB(m);
	m &= 0xF7;
	WRITE_BACK(m);
}

static void Op47(void)							// RMB4 ZP
{
	uint8_t m;
	READ_ZP_WB(m);
	m &= 0xEF;
	WRITE_BACK(m);
}

static void Op57(void)							// RMB5 ZP
{
	uint8_t m;
	READ_ZP_WB(m);
	m &= 0xDF;
	WRITE_BACK(m);
}

static void Op67(void)							// RMB6 ZP
{
	uint8_t m;
	READ_ZP_WB(m);
	m &= 0xBF;
	WRITE_BACK(m);
}

static void Op77(void)							// RMB7 ZP
{
	uint8_t m;
	READ_ZP_WB(m);
	m &= 0x7F;
	WRITE_BACK(m);
}

/*
ROL	Accumulator	ROL A		2A	1	2
Zero Page		ROL Zpg		26	2	5
Zero Page,X		ROL Zpg,X	36	2	6
Absolute		ROL Abs		2E	3	6
Absolute,X		ROL Abs,X	3E	3	7
*/

// ROL opcodes

#define OP_ROL_HANDLER(m) \
	uint8_t tmp = regs->cc & 0x01; \
	regs->cc = ((m) & 0x80 ? regs->cc | FLAG_C : regs->cc & ~FLAG_C); \
	(m) = ((m) << 1) | tmp; \
	SET_ZN((m))

static void Op2A(void)							// ROL A
{
	OP_ROL_HANDLER(regs->a);
}

static void Op26(void)							// ROL ZP
{
	uint8_t m;
	READ_ZP_WB(m);
	OP_ROL_HANDLER(m);
	WRITE_BACK(m);
}

static void Op36(void)							// ROL ZP, X
{
	uint8_t m;
	READ_ZP_X_WB(m);
	OP_ROL_HANDLER(m);
	WRITE_BACK(m);
}

static void Op2E(void)							// ROL ABS
{
	uint8_t m;
	READ_ABS_WB(m);
	OP_ROL_HANDLER(m);
	WRITE_BACK(m);
}

static void Op3E(void)							// ROL ABS, X
{
	HANDLE_PAGE_CROSSING_ABS_X;
	uint8_t m;
	READ_ABS_X_WB(m);
	OP_ROL_HANDLER(m);
	WRITE_BACK(m);
}

/*
ROR	Accumulator	ROR A		6A	1	2
Zero Page		ROR Zpg		66	2	5
Zero Page,X		ROR Zpg,X	76	2	6
Absolute		ROR Abs		6E	3	6
Absolute,X		ROR Abs,X	7E	3	7
*/

// ROR opcodes

#define OP_ROR_HANDLER(m) \
	uint8_t tmp = (regs->cc & 0x01) << 7; \
	regs->cc = ((m) & 0x01 ? regs->cc | FLAG_C : regs->cc & ~FLAG_C); \
	(m) = ((m) >> 1) | tmp; \
	SET_ZN((m))

static void Op6A(void)							// ROR A
{
	OP_ROR_HANDLER(regs->a);
}

static void Op66(void)							// ROR ZP
{
	uint8_t m;
	READ_ZP_WB(m);
	OP_ROR_HANDLER(m);
	WRITE_BACK(m);
}

static void Op76(void)							// ROR ZP, X
{
	uint8_t m;
	READ_ZP_X_WB(m);
	OP_ROR_HANDLER(m);
	WRITE_BACK(m);
}

static void Op6E(void)							// ROR ABS
{
	uint8_t m;
	READ_ABS_WB(m);
	OP_ROR_HANDLER(m);
	WRITE_BACK(m);
}

static void Op7E(void)							// ROR ABS, X
{
	HANDLE_PAGE_CROSSING_ABS_X;
	uint8_t m;
	READ_ABS_X_WB(m);
	OP_ROR_HANDLER(m);
	WRITE_BACK(m);
}

/*
RTI	Implied		RTI			40	1	6
*/

static void Op40(void)							// RTI
{
	regs->cc = regs->RdMem(0x0100 + ++regs->sp);
	regs->pc = regs->RdMem(0x0100 + ++regs->sp);
	regs->pc |= (uint16_t)(regs->RdMem(0x0100 + ++regs->sp)) << 8;
}

/*
RTS	Implied		RTS			60	1	6
*/

static void Op60(void)							// RTS
{
	regs->pc = regs->RdMem(0x0100 + ++regs->sp);
	regs->pc |= (uint16_t)(regs->RdMem(0x0100 + ++regs->sp)) << 8;
	regs->pc++;									// Since it pushes return address - 1...
}

/*
SBC	Immediate	SBC #Oper	E9	2	2
Zero Page		SBC Zpg		E5	2	3
Zero Page,X		SBC Zpg,X	F5	2	4
Absolute		SBC Abs		ED	3	4
Absolute,X		SBC Abs,X	FD	3	4
Absolute,Y		SBC Abs,Y	F9	3	4
(Zero Page,X)	SBC (Zpg,X)	E1	2	6
(Zero Page),Y	SBC (Zpg),Y	F1	2	5
(Zero Page)		SBC (Zpg)	F2	2	5
*/

// SBC opcodes

//This is non-optimal, but it works--optimize later. :-)
// We do the BCD subtraction one nybble at a time to ensure a correct result.
// 9 - m is a "Nine's Complement".  We do the BCD subtraction as a 9s
// complement addition because it's easier and it works.  :-)  Also, Decimal
// mode incurs a once cycle penalty (for the decimal correction).
#define OP_SBC_HANDLER(m) \
	uint16_t sum = (uint16_t)regs->a - (m) - (uint16_t)((regs->cc & FLAG_C) ^ 0x01); \
\
	if (regs->cc & FLAG_D) \
	{ \
		sum = (regs->a & 0x0F) + (9 - ((m) & 0x0F)) + (uint16_t)(regs->cc & FLAG_C); \
\
		if (sum > 0x09) \
			sum += 0x06; \
\
		sum += (regs->a & 0xF0) + (0x90 - ((m) & 0xF0)); \
\
		if (sum > 0x99) \
			sum += 0x60; \
\
		sum ^= 0x100; /* Invert carry, for active low borrow */ \
		regs->clock++;\
	} \
\
	regs->cc = (regs->cc & ~FLAG_C) | (((sum >> 8) ^ 0x01) & FLAG_C); \
	regs->cc = ((regs->a ^ (m)) & (regs->a ^ sum) & 0x80 ? regs->cc | FLAG_V : regs->cc & ~FLAG_V); \
	regs->a = sum & 0xFF; \
	SET_ZN(regs->a)

static void OpE9(void)							// SBC #
{
	uint16_t m = READ_IMM;
	OP_SBC_HANDLER(m);
}

static void OpE5(void)							// SBC ZP
{
	uint16_t m = READ_ZP;
	OP_SBC_HANDLER(m);
}

static void OpF5(void)							// SBC ZP, X
{
	uint16_t m = READ_ZP_X;
	OP_SBC_HANDLER(m);
}

static void OpED(void)							// SBC ABS
{
	uint16_t m = READ_ABS;
	OP_SBC_HANDLER(m);
}

static void OpFD(void)							// SBC ABS, X
{
	HANDLE_PAGE_CROSSING_ABS_X;
	uint16_t m = READ_ABS_X;
	OP_SBC_HANDLER(m);
}

static void OpF9(void)							// SBC ABS, Y
{
	HANDLE_PAGE_CROSSING_ABS_Y;
	uint16_t m = READ_ABS_Y;
	OP_SBC_HANDLER(m);
}

static void OpE1(void)							// SBC (ZP, X)
{
	uint16_t m = READ_IND_ZP_X;
	OP_SBC_HANDLER(m);
}

static void OpF1(void)							// SBC (ZP), Y
{
	HANDLE_PAGE_CROSSING_IND_Y;
	uint16_t m = READ_IND_ZP_Y;
	OP_SBC_HANDLER(m);
}

static void OpF2(void)							// SBC (ZP)
{
	uint16_t m = READ_IND_ZP;
	OP_SBC_HANDLER(m);
}

/*
SEC	Implied		SEC			38	1	2
*/

static void Op38(void)							// SEC
{
	regs->cc |= FLAG_C;
}

/*
SED	Implied		SED			F8	1	2
*/

static void OpF8(void)							// SED
{
	regs->cc |= FLAG_D;
}

/*
SEI	Implied		SEI			78	1	2
*/

static void Op78(void)							// SEI
{
	SET_I;
}

/*
The bit set and clear instructions have the form xyyy0111, where x is 0 to clear a bit or 1 to set it, and yyy is which bit at the memory location to set or clear.
   RMB0  RMB1  RMB2  RMB3  RMB4  RMB5  RMB6  RMB7
  zp  07  17  27  37  47  57  67  77
     SMB0  SMB1  SMB2  SMB3  SMB4  SMB5  SMB6  SMB7
  zp  87  97  A7  B7  C7  D7  E7  F7
*/

// SMB opcodes

static void Op87(void)							// SMB0 ZP
{
	uint8_t m;
	READ_ZP_WB(m);
	m |= 0x01;
	WRITE_BACK(m);
}

static void Op97(void)							// SMB1 ZP
{
	uint8_t m;
	READ_ZP_WB(m);
	m |= 0x02;
	WRITE_BACK(m);
}

static void OpA7(void)							// SMB2 ZP
{
	uint8_t m;
	READ_ZP_WB(m);
	m |= 0x04;
	WRITE_BACK(m);
}

static void OpB7(void)							// SMB3 ZP
{
	uint8_t m;
	READ_ZP_WB(m);
	m |= 0x08;
	WRITE_BACK(m);
}

static void OpC7(void)							// SMB4 ZP
{
	uint8_t m;
	READ_ZP_WB(m);
	m |= 0x10;
	WRITE_BACK(m);
}

static void OpD7(void)							// SMB5 ZP
{
	uint8_t m;
	READ_ZP_WB(m);
	m |= 0x20;
	WRITE_BACK(m);
}

static void OpE7(void)							// SMB6 ZP
{
	uint8_t m;
	READ_ZP_WB(m);
	m |= 0x40;
	WRITE_BACK(m);
}

static void OpF7(void)							// SMB7 ZP
{
	uint8_t m;
	READ_ZP_WB(m);
	m |= 0x80;
	WRITE_BACK(m);
}

/*
STA	Zero Page	STA Zpg		85	2	3
Zero Page,X		STA Zpg,X	95	2	4
Absolute		STA Abs		8D	3	4
Absolute,X		STA Abs,X	9D	3	5
Absolute,Y		STA Abs,Y	99	3	5
(Zero Page,X)	STA (Zpg,X)	81	2	6
(Zero Page),Y	STA (Zpg),Y	91	2	6
(Zero Page)		STA (Zpg)	92	2	5
*/

// STA opcodes

static void Op85(void)
{
	regs->WrMem(EA_ZP, regs->a);
}

static void Op95(void)
{
	regs->WrMem(EA_ZP_X, regs->a);
}

static void Op8D(void)
{
	regs->WrMem(EA_ABS, regs->a);
}

static void Op9D(void)
{
	regs->WrMem(EA_ABS_X, regs->a);
}

static void Op99(void)
{
	regs->WrMem(EA_ABS_Y, regs->a);
}

static void Op81(void)
{
	regs->WrMem(EA_IND_ZP_X, regs->a);
}

static void Op91(void)
{
	regs->WrMem(EA_IND_ZP_Y, regs->a);
}

static void Op92(void)
{
	regs->WrMem(EA_IND_ZP, regs->a);
}

/*
STX	Zero Page	STX Zpg		86	2	3
Zero Page,Y		STX Zpg,Y	96	2	4
Absolute		STX Abs		8E	3	4
*/

// STX opcodes

static void Op86(void)
{
	regs->WrMem(EA_ZP, regs->x);
}

static void Op96(void)
{
	regs->WrMem(EA_ZP_Y, regs->x);
}

static void Op8E(void)
{
	regs->WrMem(EA_ABS, regs->x);
}

/*
STY	Zero Page	STY Zpg		84	2	3
Zero Page,X		STY Zpg,X	94	2	4
Absolute		STY Abs		8C	3	4
*/

// STY opcodes

static void Op84(void)
{
	regs->WrMem(EA_ZP, regs->y);
}

static void Op94(void)
{
	regs->WrMem(EA_ZP_X, regs->y);
}

static void Op8C(void)
{
	regs->WrMem(EA_ABS, regs->y);
}

/*
STZ	Zero Page	STZ Zpg		64	2	3
Zero Page,X		STZ Zpg,X	74	2	4
Absolute		STZ Abs		9C	3	4
Absolute,X		STZ Abs,X	9E	3	5
*/

// STZ opcodes

static void Op64(void)
{
	regs->WrMem(EA_ZP, 0x00);
}

static void Op74(void)
{
	regs->WrMem(EA_ZP_X, 0x00);
}

static void Op9C(void)
{
	regs->WrMem(EA_ABS, 0x00);
}

static void Op9E(void)
{
	regs->WrMem(EA_ABS_X, 0x00);
}

/*
TAX	Implied		TAX			AA	1	2
*/

static void OpAA(void)							// TAX
{
	regs->x = regs->a;
	SET_ZN(regs->x);
}

/*
TAY	Implied		TAY			A8	1	2
*/

static void OpA8(void)							// TAY
{
	regs->y = regs->a;
	SET_ZN(regs->y);
}

/*
TRB	Zero Page	TRB Zpg		14	2	5
Absolute		TRB Abs		1C	3	6
*/

// TRB opcodes

#define OP_TRB_HANDLER(m) \
	SET_Z(m & regs->a); \
	m &= ~regs->a

static void Op14(void)							// TRB ZP
{
	uint8_t m;
	READ_ZP_WB(m);
	OP_TRB_HANDLER(m);
	WRITE_BACK(m);
}

static void Op1C(void)							// TRB ABS
{
	uint8_t m;
	READ_ABS_WB(m);
	OP_TRB_HANDLER(m);
	WRITE_BACK(m);
}

/*
TSB	Zero Page	TSB Zpg		04	2	5
Absolute		TSB Abs		0C	3	6
*/

// TSB opcodes

#define OP_TSB_HANDLER(m) \
	SET_Z(m & regs->a); \
	m |= regs->a

static void Op04(void)							// TSB ZP
{
	uint8_t m;
	READ_ZP_WB(m);
	OP_TSB_HANDLER(m);
	WRITE_BACK(m);
}

static void Op0C(void)							// TSB ABS
{
	uint8_t m;
	READ_ABS_WB(m);
	OP_TSB_HANDLER(m);
	WRITE_BACK(m);
}

/*
TSX	Implied		TSX			BA	1	2
*/

static void OpBA(void)							// TSX
{
	regs->x = regs->sp;
	SET_ZN(regs->x);
}

/*
TXA	Implied		TXA			8A	1	2
*/

static void Op8A(void)							// TXA
{
	regs->a = regs->x;
	SET_ZN(regs->a);
}

/*
TXS	Implied		TXS			9A	1	2
*/

static void Op9A(void)							// TXS
{
	regs->sp = regs->x;
}

/*
TYA	Implied		TYA			98	1	2
*/
static void Op98(void)							// TYA
{
	regs->a = regs->y;
	SET_ZN(regs->a);
}

static void Op__(void)
{
	regs->cpuFlags |= V65C02_STATE_ILLEGAL_INST;
}

//
// Ok, the exec_op[] array is globally defined here basically to save
// a LOT of unnecessary typing.  Sure it's ugly, but hey, it works!
//
static void (* exec_op[256])() = {
	Op00, Op01, Op__, Op__, Op04, Op05, Op06, Op07, Op08, Op09, Op0A, Op__, Op0C, Op0D, Op0E, Op0F,
	Op10, Op11, Op12, Op__, Op14, Op15, Op16, Op17, Op18, Op19, Op1A, Op__, Op1C, Op1D, Op1E, Op1F,
	Op20, Op21, Op__, Op__, Op24, Op25, Op26, Op27, Op28, Op29, Op2A, Op__, Op2C, Op2D, Op2E, Op2F,
	Op30, Op31, Op32, Op__, Op34, Op35, Op36, Op37, Op38, Op39, Op3A, Op__, Op3C, Op3D, Op3E, Op3F,
	Op40, Op41, Op__, Op__, Op__, Op45, Op46, Op47, Op48, Op49, Op4A, Op__, Op4C, Op4D, Op4E, Op4F,
	Op50, Op51, Op52, Op__, Op__, Op55, Op56, Op57, Op58, Op59, Op5A, Op__, Op__, Op5D, Op5E, Op5F,
	Op60, Op61, Op__, Op__, Op64, Op65, Op66, Op67, Op68, Op69, Op6A, Op__, Op6C, Op6D, Op6E, Op6F,
	Op70, Op71, Op72, Op__, Op74, Op75, Op76, Op77, Op78, Op79, Op7A, Op__, Op7C, Op7D, Op7E, Op7F,
	Op80, Op81, Op__, Op__, Op84, Op85, Op86, Op87, Op88, Op89, Op8A, Op__, Op8C, Op8D, Op8E, Op8F,
	Op90, Op91, Op92, Op__, Op94, Op95, Op96, Op97, Op98, Op99, Op9A, Op__, Op9C, Op9D, Op9E, Op9F,
	OpA0, OpA1, OpA2, Op__, OpA4, OpA5, OpA6, OpA7, OpA8, OpA9, OpAA, Op__, OpAC, OpAD, OpAE, OpAF,
	OpB0, OpB1, OpB2, Op__, OpB4, OpB5, OpB6, OpB7, OpB8, OpB9, OpBA, Op__, OpBC, OpBD, OpBE, OpBF,
	OpC0, OpC1, Op__, Op__, OpC4, OpC5, OpC6, OpC7, OpC8, OpC9, OpCA, Op__, OpCC, OpCD, OpCE, OpCF,
	OpD0, OpD1, OpD2, Op__, Op__, OpD5, OpD6, OpD7, OpD8, OpD9, OpDA, Op__, Op__, OpDD, OpDE, OpDF,
	OpE0, OpE1, Op__, Op__, OpE4, OpE5, OpE6, OpE7, OpE8, OpE9, OpEA, Op__, OpEC, OpED, OpEE, OpEF,
	OpF0, OpF1, OpF2, Op__, Op__, OpF5, OpF6, OpF7, OpF8, OpF9, OpFA, Op__, Op__, OpFD, OpFE, OpFF
};


/*
FCA8: 38        698  WAIT     SEC
FCA9: 48        699  WAIT2    PHA
FCAA: E9 01     700  WAIT3    SBC   #$01
FCAC: D0 FC     701           BNE   WAIT3      ;1.0204 USEC
FCAE: 68        702           PLA              ;(13+27/2*A+5/2*A*A)
FCAF: E9 01     703           SBC   #$01
FCB1: D0 F6     704           BNE   WAIT2
FCB3: 60        705           RTS

FBD9: C9 87     592  BELL1    CMP   #$87       ;BELL CHAR? (CNTRL-G)
FBDB: D0 12     593           BNE   RTS2B      ;  NO, RETURN
FBDD: A9 40     594           LDA   #$40       ;DELAY .01 SECONDS
FBDF: 20 A8 FC  595           JSR   WAIT
FBE2: A0 C0     596           LDY   #$C0
FBE4: A9 0C     597  BELL2    LDA   #$0C       ;TOGGLE SPEAKER AT
FBE6: 20 A8 FC  598           JSR   WAIT       ;  1 KHZ FOR .1 SEC.
FBE9: AD 30 C0  599           LDA   SPKR
FBEC: 88        600           DEY
FBED: D0 F5     601           BNE   BELL2
FBEF: 60        602  RTS2B    RTS
*/
//int instCount[256];
#ifdef __DEBUG__
bool dumpDis = false;
//bool dumpDis = true;
#endif

/*
On //e, $FCAA is the delay routine. (seems to not have changed from ][+)
*/

#define DO_BACKTRACE
#ifdef DO_BACKTRACE
#define BACKTRACE_SIZE 16384
uint32_t btQueuePtr = 0;
V65C02REGS btQueue[BACKTRACE_SIZE];
uint8_t btQueueInst[BACKTRACE_SIZE][4];
#endif

//
// Function to execute 65C02 for "cycles" cycles
//
//static bool first = true;
void Execute65C02(V65C02REGS * context, uint32_t cycles)
{
	regs = context;

	// Calculate number of clock cycles to run for
	uint64_t endCycles = regs->clock + (uint64_t)cycles - regs->overflow;

	while (regs->clock < endCycles)
	{
// Ultima I (WTF? This *used* to work! >:-U) [Now it does...  :-P]
// Turns out it was a problem with PAGE2 changing too much (it ignored the HIRES switch when switching memory, causing code at $2141 to be swapped out with zeroes).
/*if (regs->pc == 0xC311)
	dumpDis = true;
else if (regs->pc == 0x2141)
	dumpDis = false;*/

#if 0
// Bard's Tale II
// Turns out a bug in floppydrive.cpp prevented this from working.  :-P
static bool hitGo = false;

if (regs->pc == 0xA000)
{
	dumpDis = true;
	hitGo = true;
}

// $FCA8 also needs to be silenced too ($FCB3 is exit point)
if (regs->pc == 0xA181 && hitGo)
{
	dumpDis = false;
	WriteLog("*** BT2 DELAY\n");
}
else if (regs->pc == 0xA18B && hitGo)
{
	dumpDis = true;
}
else if (regs->pc == 0xFCA8 && hitGo)
{
	dumpDis = false;
	WriteLog("*** MONITOR DELAY ($FCA8)\n");
}
else if (regs->pc == 0xFCB3 && hitGo)
{
	dumpDis = true;
}
else if (regs->pc == 0xBD11 && hitGo)
{
	dumpDis = false;
	WriteLog("*** BT2 DELAY $BD11\n");
}
else if (regs->pc == 0xDB1E && hitGo)
{
	dumpDis = true;
}
else if (regs->pc == 0xA003 && hitGo)
{
	dumpDis = false;
	WriteLog("*** BT2 CHECK FOR $B7s\n");
}
else if (regs->pc == 0xA063 && hitGo)
{
	dumpDis = true;
}
else if (regs->pc == 0xA0FE && hitGo)
{
	dumpDis = false;
	WriteLog("*** BT2 CHECK FOR $D5 $AA $96 HEADER\n");
}
else if (regs->pc == 0xA112 && hitGo)
{
	dumpDis = true;
	WriteLog("*** BT2 LOOK FOR HEADER FAILED\n");
}
else if (regs->pc == 0xA14B && hitGo)
{
	dumpDis = true;
}
else if (regs->pc == 0xA254 && hitGo)
{
	// This is where it hits a BRK and goes BOOM
	dumpDis = false;
	hitGo = false;
}
else if (regs->pc == 0xA155)
{
	WriteLog("*** $A2E2 is %02X...\n", regs->RdMem(0xA2E2));
}
else if (regs->pc == 0xA1C2)
{
	static char bcName[13][5] = { "JMP", "JMPA", "BNE", "LDI", "JSR", "LDA", "SUB", "STA", "RTS", "JMPA", "INC", "CRSH", "ILDA" };
	static int bcLen[13] = { 2, 2, 2, 1, 2, 2, 1, 2, 0, 2, 2, 0, 2 };

	uint16_t addr = RdMemWZP(0x52) + regs->y;
	uint8_t bytecode = regs->RdMem(addr);
	uint16_t bcAddr = ((regs->RdMem(addr + 2) ^ 0xD9) << 8) | (regs->RdMem(addr + 1) ^ 0x03);
	uint8_t bcVal = regs->RdMem(addr + 1) ^ 0x4C;

	WriteLog("\n*** bc %04X: %s ", addr, bcName[bytecode]);

	if (bcLen[bytecode] == 1)
		WriteLog("$%02X", bcVal);
	else if (bcLen[bytecode] == 2)
		WriteLog("$%04X", bcAddr);

	WriteLog("\n\n");
}
#endif

#if 0
// Border Zone timing...
static bool inDelay1 = false;
static bool inDelay2 = false;
static bool inRead1 = false;
static bool hitGo = false;
if (regs->pc == 0xF0B1)
	WriteLog("*** $F09C ($6F,70) -> $%02X%02X\n", regs->RdMem(0x70), regs->RdMem(0x6F));

if (regs->pc == 0xC8F2)
	hitGo = true;

// Delay is $D20D to $D21D...
if (regs->pc == 0xD20D && hitGo && !inDelay1)
{
	dumpDis = false;
	inDelay1 = true;
	WriteLog("*** DELAY\n");
}
else if (regs->pc == 0xD21D && inDelay1)
{
	dumpDis = true;
	inDelay1 = false;
}

// Next delay starts @ $D356 - $D36A
else if (regs->pc == 0xD356 && hitGo && !inDelay2)
{
	dumpDis = false;
	inDelay2 = true;
	WriteLog("*** DELAY #2\n");
}
else if (regs->pc == 0xD36A && inDelay2)
{
	dumpDis = true;
	inDelay2 = false;
}
else if (regs->pc == 0xD486 && hitGo && !inRead1)
{
	dumpDis = false;
	inRead1 = true;
	WriteLog("\n*** FAST READ ROUTINE (!!!)\n\n");
}
else if (regs->pc == 0xD4B1 && inRead1)
{
	dumpDis = true;
	inRead1 = false;
}
#endif
#if 0
// 13-sector disk debugging
// start with the slot ROM
static bool inDelay = false;
static bool inBell = false;
static bool inReadSector = false;
static bool inSlotROM = false;
if (regs->pc == 0xFCA8)// && !inSlotROM)//!inBell && !inReadSector)
{
	dumpDis = false;
	inDelay = true;
	WriteLog("*** DELAY\n");
}
else if (regs->pc == 0xFCB3 && inDelay && inSlotROM)//&& !inBell && !inReadSector)
{
	dumpDis = true;
	inDelay = false;
}
if (regs->pc == 0xFBD9)
{
	dumpDis = false;
	inBell = true;
	WriteLog("*** BELL1\n");
}
else if (regs->pc == 0xFBEF && inBell)
{
//	dumpDis = true;
	inBell = false;
}
//else if (regs->pc == 0xC664)
else if (regs->pc == 0xC663)
{
	dumpDis = true;
	inSlotROM = true;
	WriteLog("*** DISK @ $C600\n");
}
else if (regs->pc == 0x801)
{
	WriteLog("*** DISK @ $801\n");
	dumpDis = true;
}
#endif
// Hard disk debugging
#if 0
if (first && (regs->pc == 0x801))
{
//	regs->WrMem(0x42, 1); // v3.0 does this now...
	regs->WrMem(0x44, 0); // who writes non-zero to here??? (AHSSC does)
	first = false;
//	dumpDis = true;
//WriteLog("V65C02: Executing $801...\n");
}
else if (regs->pc == 0x869)
{
/*	regs->WrMem(0x42, 1);
	first = false;//*/
/*	static char disbuf[80];
	uint16_t pc=0x801;
	while (pc < 0xA00)
	{
		pc += Decode65C02(regs, disbuf, pc);
		WriteLog("%s\n", disbuf);
	}*/
/*	dumpDis = true;
	WriteLog("\n>>> $42-7: %02X %02X %02X %02X %02X %02X\n\n", regs->RdMem(0x42), regs->RdMem(0x43), regs->RdMem(0x44), regs->RdMem(0x45), regs->RdMem(0x46), regs->RdMem(0x47));//*/
}
#endif
#if 0
//Epoch
if (regs->pc == 0x0518)
{
	dumpDis = true;
}
else if (regs->pc == 0x051E)
{
	uint16_t c1 = regs->RdMem(0xFF);
	uint16_t c2 = regs->RdMem(0x00);
	WriteLog("$FF/$00 = $%02X $%02X\n", c1, c2);
	WriteLog("--> $%02X\n", regs->RdMem((c2 << 8) | c1));
}
else if (regs->pc == 0x0522)
{
	uint16_t c1 = regs->RdMem(0xFF);
	uint16_t c2 = regs->RdMem(0x00);
	WriteLog("$FF/$00 = $%02X $%02X\n", c1, c2);
	WriteLog("--> $%02X\n", regs->RdMem(((c2 << 8) | c1) + 1));
}
#endif
#if 0
// Up N Down testing
// Now Ankh testing...
static bool inDelay = false;
static bool inBell = false;
static bool inReadSector = false;
if (regs->pc == 0xFCA8 && !inBell && !inReadSector)
{
	dumpDis = false;
	inDelay = true;
	WriteLog("*** DELAY\n");
}
else if (regs->pc == 0xFCB3 && inDelay && !inBell && !inReadSector)
{
	dumpDis = true;
	inDelay = false;
}
if (regs->pc == 0xFBD9)
{
	dumpDis = false;
	inBell = true;
	WriteLog("*** BELL1\n");
}
else if (regs->pc == 0xFBEF && inBell)
{
	dumpDis = true;
	inBell = false;
}
else if (regs->pc == 0xC600)
{
	dumpDis = false;
	WriteLog("*** DISK @ $C600\n");
}
else if (regs->pc == 0x801)
{
	WriteLog("*** DISK @ $801\n");
	dumpDis = true;
}
else if (regs->pc == 0xC119)
{
	dumpDis = false;
	WriteLog("*** BIOS @ $C119\n");
}
else if (regs->pc == 0xC117)
{
	dumpDis = true;
}
else if (regs->pc == 0x843)
{
	dumpDis = false;
	inReadSector = true;
	uint16_t lo = regs->RdMem(0x26);
	uint16_t hi = regs->RdMem(0x27);
	WriteLog("\n*** DISK Read sector ($26=$%04X)...\n\n", (hi << 8) | lo);
}
else if (regs->pc == 0x8FC)
{
	dumpDis = true;
	inReadSector = false;
}
else if (regs->pc == 0xA8A8 || regs->pc == 0xC100)
{
	dumpDis = false;
}
else if (regs->pc == 0x8FD)
{
//	regs->WrMem(0x827, 3);
//	regs->WrMem(0x82A, 0);
//1 doesn't work, but 2 does (only with WOZ, not with DSK; DSK needs 4)...
//	regs->WrMem(0x0D, 4);
}

#endif
#if 0
static bool inDelay = false;
static bool inMLI = false;
static uint16_t mliReturnAddr = 0;
static uint8_t mliCmd = 0;
if (regs->pc == 0x160B && dumpDis)
{
	inDelay = true;
	dumpDis = false;
	WriteLog("*** DELAY\n");
}
else if (regs->pc == 0x1616 && inDelay)
{
	inDelay = false;
	dumpDis = true;
}
else if (regs->pc == 0xD385 && dumpDis)
{
	inDelay = true;
	dumpDis = false;
	WriteLog("*** DELAY\n");
}
else if (regs->pc == 0xD397 && inDelay)
{
	inDelay = false;
	dumpDis = true;
}
else if (regs->pc == 0xBF00 && dumpDis)
{
	uint16_t lo = regs->RdMem(regs->sp + 0x101);
	uint16_t hi = regs->RdMem(regs->sp + 0x102);
	mliReturnAddr = ((hi << 8) | lo) + 1;
	mliCmd = regs->RdMem(mliReturnAddr);
	WriteLog("*** Calling ProDOS MLI with params: %02X %04X\n", mliCmd, RdMemW(mliReturnAddr + 1));
	mliReturnAddr += 3;
	inMLI = true;

	// We want to see what's going on in the WRITE BLOCK command... :-P
//	if (mliCmd != 0x81)
//		dumpDis = false;
}
else if (regs->pc == mliReturnAddr && inMLI)
{
//extern bool stopWriting;
//Stop writing to disk after the first block is done
//	if (mliCmd == 0x81)
//		stopWriting = true;

	inMLI = false;
	dumpDis = true;
}
else if (regs->pc == 0xAB3A && dumpDis && !inDelay)
{
	dumpDis = false;
	inDelay = true;
	WriteLog("\n*** DELAY (A=$%02X)\n\n", regs->a);
}
else if (regs->pc == 0xAB4A && inDelay)
{
	dumpDis = true;
	inDelay = false;
}

if (regs->pc == 0xA80B)
	dumpDis = true;

#endif
#if 0
static bool weGo = false;
static bool inDelay = false;
if (regs->pc == 0x92BA)
{
	dumpDis = true;
	weGo = true;
}
else if (regs->pc == 0xAB3A && weGo && !inDelay)
{
	dumpDis = false;
	inDelay = true;
	WriteLog("\n*** DELAY (A=$%02X)\n\n", regs->a);
}
else if (regs->pc == 0xAB4A && weGo)
{
	dumpDis = true;
	inDelay = false;
}
else if (regs->pc == 0xA8B5 && weGo)
{
	WriteLog("\n$D4=%02X, $AC1F=%02X, $AC20=%02X\n\n", regs->RdMem(0xD4), regs->RdMem(0xAC1F), regs->RdMem(0xAC20));
}
/*else if (regs->pc == 0xA8C4 && weGo)
{
	WriteLog("Cheating... (clearing Carry flag)\n");
	regs->cc &= ~FLAG_C;
}*/
#endif
#if 0
static bool weGo = false;
if (regs->pc == 0x80AE)
{
	dumpDis = true;
	weGo = true;
}
else if (regs->pc == 0xFCA8 && weGo)
{
	dumpDis = false;
	WriteLog("\n*** DELAY (A=$%02X)\n\n", regs->a);
}
else if (regs->pc == 0xFCB3 && weGo)
{
	dumpDis = true;
}
#endif
#if 0
/*if (regs->pc == 0x4007)
{
	dumpDis = true;
}//*/
if (regs->pc == 0x444B)
{
	WriteLog("\n*** End of wait...\n\n");
	dumpDis = true;
}//*/
if (regs->pc == 0x444E)
{
	WriteLog("\n*** Start of wait...\n\n");
	dumpDis = false;
}//*/
#endif
/*if (regs->pc >= 0xC600 && regs->pc <=0xC6FF)
{
	dumpDis = true;
}
else
	dumpDis = false;//*/
/*if (regs->pc == 0xE039)
{
	dumpDis = true;
}//*/

#if 0
/*if (regs->pc == 0x0801)
{
	WriteLog("\n*** DISK BOOT subroutine...\n\n");
	dumpDis = true;
}//*/
if (regs->pc == 0xE000)
{
#if 0
	WriteLog("\n*** Dump of $E000 routine ***\n\n");

	for(uint32_t addr=0xE000; addr<0xF000;)
	{
		addr += Decode65C02(addr);
		WriteLog("\n");
	}
#endif
	WriteLog("\n*** DISK part II subroutine...\n\n");
	dumpDis = true;
}//*/
if (regs->pc == 0xD000)
{
	WriteLog("\n*** CUSTOM DISK READ subroutine...\n\n");
	dumpDis = false;
}//*/
if (regs->pc == 0xD1BE)
{
//	WriteLog("\n*** DISK part II subroutine...\n\n");
	dumpDis = true;
}//*/
if (regs->pc == 0xD200)
{
	WriteLog("\n*** CUSTOM SCREEN subroutine...\n\n");
	dumpDis = false;
}//*/
if (regs->pc == 0xD269)
{
//	WriteLog("\n*** DISK part II subroutine...\n\n");
	dumpDis = true;
}//*/
#endif
//if (regs->pc == 0xE08E)
/*if (regs->pc == 0xAD33)
{
	WriteLog("\n*** After loader ***\n\n");
	dumpDis = true;
}//*/
/*if (regs->pc == 0x0418)
{
	WriteLog("\n*** CUSTOM DISK READ subroutine...\n\n");
	dumpDis = false;
}
if (regs->pc == 0x0)
{
	dumpDis = true;
}//*/
#ifdef __DEBUGMON__
//WAIT is commented out here because it's called by BELL1...
if (regs->pc == 0xFCA8)
{
	WriteLog("\n*** WAIT subroutine...\n\n");
	dumpDis = false;
}//*/
if (regs->pc == 0xFBD9)
{
	WriteLog("\n*** BELL1 subroutine...\n\n");
//	dumpDis = false;
}//*/
if (regs->pc == 0xFC58)
{
	WriteLog("\n*** HOME subroutine...\n\n");
//	dumpDis = false;
}//*/
if (regs->pc == 0xFDED)
{
	WriteLog("\n*** COUT subroutine...\n\n");
	dumpDis = false;
}
#endif
#if 0
// ProDOS debugging
if (regs->pc == 0x2000)
	dumpDis = true;
#endif

#ifdef __DEBUG__
#ifdef DO_BACKTRACE
//uint32_t btQueuePtr = 0;
//V65C02REGS btQueue[BACKTRACE_SIZE];
//uint8_t btQueueInst[BACKTRACE_SIZE][4];
memcpy(&btQueue[btQueuePtr], regs, sizeof(V65C02REGS));
btQueuePtr = (btQueuePtr + 1) % BACKTRACE_SIZE;
#endif
#endif
#ifdef __DEBUG__
static uint16_t spc, ppc = 0;
static bool seenHi = false;
static uint64_t oldClock = 0;
spc = regs->pc;
static char disbuf[80];
if (dumpDis)
{
	Decode65C02(regs, disbuf, regs->pc);
	WriteLog("%s", disbuf);
}
#endif
		uint8_t opcode = regs->RdMem(regs->pc++);

#if 0
if (opcode == 0)
//if (regs->pc == 0xA255)
//static bool seenBT = false;
//if (hitGo && !seenBT)
//if (dobacktrace)
{
//seenBT = true;
	static char disbuf[80];
//	uint32_t btStart = btQueuePtr - 12 + (btQueuePtr < 12 ? BACKTRACE_SIZE : 0);
	uint32_t btStart = 0;

	for(uint32_t i=btStart; i<btQueuePtr; i++)
	{
		Decode65C02(regs, disbuf, btQueue[i].pc);
		WriteLog("%s\n", disbuf);
	}
}
#endif
//if (!(regs->cpuFlags & V65C02_STATE_ILLEGAL_INST))
//instCount[opcode]++;

		if (regs->Timer)
			regs->Timer(CPUCycles[opcode]);

		uint64_t clockSave = regs->clock + CPUCycles[opcode];

		// We need this because the opcode function could add 1 or 2 cycles
		// to regs->clock which aren't accounted for in CPUCycles[].
//		uint64_t clockSave = regs->clock;

		// Execute that opcode...
		exec_op[opcode]();
		regs->clock += CPUCycles[opcode];

		// Tell the timer function (if any) how many PHI2s have elapsed...
//		if (regs->Timer)
		if (regs->Timer && (regs->clock - clockSave) > 0)
			regs->Timer(regs->clock - clockSave);

#ifdef __DEBUG__
if (dumpDis)
{
	WriteLog(" [SP=01%02X, CC=%s%s.%s%s%s%s%s, A=%02X, X=%02X, Y=%02X](%d)[%02X]\n",//<%s>\n",
		regs->sp,
		(regs->cc & FLAG_N ? "N" : "-"), (regs->cc & FLAG_V ? "V" : "-"),
		(regs->cc & FLAG_B ? "B" : "-"), (regs->cc & FLAG_D ? "D" : "-"),
		(regs->cc & FLAG_I ? "I" : "-"), (regs->cc & FLAG_Z ? "Z" : "-"),
		(regs->cc & FLAG_C ? "C" : "-"), regs->a, regs->x, regs->y, regs->clock - clockSave + CPUCycles[opcode], floppyDrive[0].dataRegister);//, sequence);
	sequence[0] = 0;

	if (((spc == 0xD4D1) || (spc == 0xD4E2)) && (floppyDrive[0].dataRegister & 0x80))
		seenHi = true;

	if ((spc == 0xD4CE) || (spc == 0xD4DF))
	{
		WriteLog(" (%d)\n", regs->clock - oldClock);

		if ((regs->y & 0x80) == 0 && seenHi && ((ppc == 0xD4D1) || (ppc == 0xD4E2)))
			WriteLog("\n***** MISS! *****\n\n");

		seenHi = false;
		oldClock = regs->clock;
	}

	ppc = spc;
}
#endif

#ifdef __DEBUGMON__
if (regs->pc == 0xFCB3)	// WAIT exit point
{
	dumpDis = true;
}//*/
/*if (regs->pc == 0xFBEF)	// BELL1 exit point
{
	dumpDis = true;
}//*/
/*if (regs->pc == 0xFC22)	// HOME exit point
{
	dumpDis = true;
}//*/
if (regs->pc == 0xFDFF)	// COUT exit point
{
	dumpDis = true;
}
if (regs->pc == 0xFBD8)
{
	WriteLog("\n*** BASCALC set BASL/H = $%04X\n\n", RdMemW(0x0028));
}//*/
#endif

//These should be correct now...
		if (regs->cpuFlags & V65C02_ASSERT_LINE_RESET)
		{
			// Not sure about this...
			regs->sp = 0xFF;
			regs->cc = FLAG_I;				// Reset the CC register
			regs->pc = RdMemW(0xFFFC);		// And load PC with RESET vector

			regs->cpuFlags = 0;				// Clear CPU flags...
#ifdef __DEBUG__
WriteLog("\n*** RESET *** (PC = $%04X)\n\n", regs->pc);
#endif
		}
		else if (regs->cpuFlags & V65C02_ASSERT_LINE_NMI)
		{
#ifdef __DEBUG__
WriteLog("\n*** NMI ***\n\n");
#endif
			regs->WrMem(0x0100 + regs->sp--, regs->pc >> 8);	// Save PC & CC
			regs->WrMem(0x0100 + regs->sp--, regs->pc & 0xFF);
			regs->WrMem(0x0100 + regs->sp--, regs->cc);
			SET_I;
			CLR_D;
			regs->pc = RdMemW(0xFFFA);		// Jump to NMI vector

			regs->clock += 7;
			regs->cpuFlags &= ~V65C02_ASSERT_LINE_NMI;	// Reset NMI line
		}
		else if ((regs->cpuFlags & V65C02_ASSERT_LINE_IRQ)
			// IRQs are maskable, so check if the I flag is clear
			&& (!(regs->cc & FLAG_I)))
		{
#ifdef __DEBUG__
WriteLog("\n*** IRQ ***\n\n");
WriteLog("Clock=$%X\n", regs->clock);
//dumpDis = true;
#endif
			regs->WrMem(0x0100 + regs->sp--, regs->pc >> 8);	// Save PC & CC
			regs->WrMem(0x0100 + regs->sp--, regs->pc & 0xFF);
			regs->WrMem(0x0100 + regs->sp--, regs->cc);
			SET_I;
			CLR_D;
			regs->pc = RdMemW(0xFFFE);		// Jump to IRQ vector

			regs->clock += 7;
			regs->cpuFlags &= ~V65C02_ASSERT_LINE_IRQ;	// Reset IRQ line
		}
	}

	// If we went longer than the passed in cycles, make a note of it so we can
	// subtract it out from a subsequent run.  It's guaranteed to be non-
	// negative, because the condition that exits the main loop above is
	// written such that regs->clock has to be equal or larger than endCycles
	// to exit from it.
	regs->overflow = regs->clock - endCycles;
}
