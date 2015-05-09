#ifndef PARAMETER_H
#define PARAMETER_H

#define ADD 0x20
#define SUB 0x22
#define AND 0x24
#define OR 0x25
#define XOR 0x26
#define NOR 0x27
#define NAND 0x28
#define SLT 0x2A
#define SLL 0x00
#define SRL 0x02
#define SRA 0x03
#define JR 0x08

#define ADDI 0x08
#define LW 0x23
#define LH 0x21
#define LHU 0x25
#define LB 0x20
#define LBU 0x24
#define SW 0x2B
#define SH 0x29
#define SB 0x28
#define LUI 0x0F
#define ANDI 0x0C
#define ORI 0x0D
#define NORI 0x0E
#define SLTI 0x0A
#define BEQ 0x04
#define BNE 0x05

#define J 0x02
#define JAL 0x03

#define HALT 0x3F
#define ZERO 0x00000000

#define MEM_SIZE 1024
#define WRITE_ZERO 0
#define NUMBER_OVERFLOW 1
#define ADDRESS_OVERFLOW 2
#define MISALIGNMENT 3

extern unsigned int imem[MEM_SIZE/4];
extern unsigned int dmem[MEM_SIZE/4];
extern unsigned int sp;
extern int iSize, dSize;
extern unsigned int reg[32];
extern unsigned int PC, cycle;
extern bool error_halt;
extern FILE *snapshot;
extern FILE *error_dump;

//struct pipeline;
//typedef struct pipeline PiReg;
typedef struct pipeline{

	bool Jump, Branch, BranchTaken; //ID

	int ALUOp;
	int RegDst;//0:rt  1:rd  2:31
	bool ALUSrc;//EXE

	bool MemRead, MemWrite; //MEM

	bool RegWrite, MemtoReg; //WB

	unsigned int instr;
	unsigned int PCPlus4;
	unsigned int rs, rt, rd, writeReg;
	unsigned int RegData1, RegData2, shamt, SignImm;

	unsigned int ALUOut;//from EX
	unsigned int WriteData, ReadData;//for DM


}PiReg;
extern PiReg IF_ID, ID_EX, EX_MEM, MEM_WB;
extern char IF_str[50], ID_str[50], EX_str[50], DM_str[50], WB_str[50];
#endif


