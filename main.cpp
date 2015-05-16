#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "parameter.h"
#include "error.h"
#include "load.h"
#include "output.h"
unsigned int imem[MEM_SIZE/4];
unsigned int dmem[MEM_SIZE/4];
unsigned int sp;
int iSize, dSize;
unsigned int reg[32];
unsigned int PC, cycle;
FILE *snapshot, *error_dump;
PiReg IF_ID, ID_EX, EX_MEM, MEM_WB;
PiReg ID_tmp, EX_tmp, DM_tmp;
char IF_str[50], ID_str[50], EX_str[50], DM_str[50], WB_str[50];
unsigned int WB_Result;
bool error_halt;
bool load_hazard;
int haltCnt;

void IF_stage(int*);
void ID_stage(void);
void EX_stage(void);
void DM_stage(void);
void WB_stage(void);
void updateReg(void);//update the register between stages
bool MEM_hazard(unsigned int);
bool EX_hazard(unsigned int);

bool branch_stall_check(int , int);
void branch_fwd_check(int, int);

int main()
{
    if(!load_image()){
        puts("Cannot load the images");
        return 0;
    }
    init();

    cycle = 0;
    error_halt = false;

    int i = PC/4;

    while( i < MEM_SIZE/4){
		haltCnt = 0;
		print_cycle();
		/*if( (imem[i]>>26)==HALT){
            haltCnt++;
            printf("cycle: %d %s %08X\n",cycle, instr_toString(imem[i]), imem[i]);
		}*/

		load_hazard = false;

		cycle++;


		WB_stage();

		DM_stage();

		EX_stage();

		ID_stage();

		IF_stage(&i);


		updateReg();

		print_pipeline_stage();

        if(error_halt || haltCnt==5){
            break;
        }
    }
    //printf("%08X  %08X  %08X\n", imem[0x28/4], imem[0x34/4], imem[0x38/4]);
   // printf("haltCnt:%d  error_halt:%d\n",haltCnt, error_halt);
   // printf("%d\n",cycle);
    fclose(snapshot);
    fclose(error_dump);
    return 0;
}
void updateReg(void)
{
	MEM_WB = DM_tmp;
	EX_MEM = EX_tmp;
	ID_EX = ID_tmp;
}
void WB_stage(void)
{
	unsigned int writeReg, instr;
	writeReg = MEM_WB.writeReg;
	instr = MEM_WB.instr;
    instr_toString(WB_str, instr);

	if( (instr>>26)==HALT){
        haltCnt++;
        return;
    }

	if( (instr>>26)==JAL ){
		WB_Result = MEM_WB.PCPlus4;
	}
	else if(MEM_WB.MemtoReg){
		WB_Result = MEM_WB.ReadData;
	}
	else{
		WB_Result = MEM_WB.ALUOut;
	}
	if(MEM_WB.RegWrite) {
		/*if(cycle==5){
            printf("%s %d  %d\n",instr_toString(instr),is_nop(instr), writeReg==0);
		}*/
		if(!is_nop(instr) &&  writeReg==0){
			print_error(WRITE_ZERO, cycle);
		}
		else {
            reg[writeReg] = WB_Result;

        }
	}
}
void DM_stage(void)
{
	unsigned int ALUOut, WriteData, ReadData;
	unsigned int writeReg;
	int opcode = (EX_MEM.instr)>>26;
	unsigned int masks[4] = {0x00ffffff, 0xff00ffff, 0xffff00ff, 0xffffff00};
	unsigned int utmp;
	int stmp, shift, save;
    unsigned int instr = EX_MEM.instr;
	instr_toString(DM_str,instr);
	if( (instr>>26)==HALT) {
		DM_tmp.instr = 0xffffffff;
		haltCnt++;
        return;
    }

	ALUOut = EX_MEM.ALUOut;
	WriteData = EX_MEM.WriteData;
	writeReg = EX_MEM.writeReg;

	if(EX_MEM.MemRead || EX_MEM.MemWrite)
        check_DM_errors(opcode, ALUOut);
	if(error_halt) return;

	if(EX_MEM.MemRead){
		switch(opcode){ //TODO: overflow, misalign
			case LW:
				ReadData = dmem[ALUOut/4]; break;
			case LH:
				stmp = dmem[ALUOut/4];
				ReadData = ALUOut%4==0 ? stmp>>16 : (stmp<<16)>>16;
				break;
			case LHU:
				utmp = dmem[ALUOut/4];
				ReadData = ALUOut%4==0 ? utmp>>16 : (utmp<<16)>>16;
				break;
			case LB:
				stmp = dmem[ALUOut/4];
				shift = (ALUOut%4)*8;
				ReadData = (stmp << shift) >> 24;
				break;
			case LBU:
				utmp = dmem[ALUOut/4];
				shift = (ALUOut%4)*8;
				ReadData = (utmp << shift) >> 24;
				break;
		}
	}
	else if(EX_MEM.MemWrite){//TODO: overflow, misalign
		switch(opcode){
			case SW:
				dmem[ALUOut/4] = WriteData;
				break;
			case SH:
				utmp = dmem[ALUOut/4];
				save = WriteData & 0x0000ffff;
				if(ALUOut%4==0){
					dmem[ALUOut/4] = (utmp&0x0000ffff) + (save<<16);
				}else{
					dmem[ALUOut/4] = ((utmp>>16)<<16) + save;
				}
				break;
			case SB:
			    utmp = dmem[ALUOut/4];
				save = WriteData & 0x000000ff;
				shift = 24 - (ALUOut%4)*8;
				dmem[ALUOut/4] = (utmp & masks[ALUOut%4]) + (save<<shift);
				break;
		}
	}

	DM_tmp.RegWrite = EX_MEM.RegWrite;
	DM_tmp.MemtoReg = EX_MEM.MemtoReg;

	DM_tmp.rs = EX_MEM.rs;
	DM_tmp.rt = EX_MEM.rt;
	DM_tmp.rd = EX_MEM.rd;

	DM_tmp.ALUOut = ALUOut;
	DM_tmp.ReadData = ReadData;
	DM_tmp.writeReg = writeReg;
	DM_tmp.instr = instr;
	DM_tmp.PCPlus4 = EX_MEM.PCPlus4;
}
void EX_stage(void)
{
	unsigned int rs, rt, rd, shamt, writeReg, instr;
	unsigned int SrcA, SrcB, WriteData;
	rs = ID_EX.rs;
	rt = ID_EX.rt;
	rd = ID_EX.rd;
	shamt = ID_EX.shamt;
	instr = ID_EX.instr;
	instr_toString(EX_str,instr);
	int opcode = (instr>>26);
	int funct = (instr<<26)>>26;
	if( opcode==HALT){
		EX_tmp.instr = 0xffffffff;
		haltCnt++;
        return;
    }

	switch(ID_EX.RegDst){
		case 0: writeReg = rt; break;
		case 1: writeReg = rd; break;
		case 2: writeReg = 31; break;
		default: puts("RegDst error!");
	}

	//Forward不包含branch, jump, LUI
	if(!ID_EX.Jump && !ID_EX.Branch && ID_EX.ALUOp!=LUI){
		//ForwardA 不包含sll srl sra
		if( MEM_hazard(rs) && !(opcode==0x00 && (funct==SLL||funct==SRL||funct==SRA)) ){
			SrcA = WB_Result;
			sprintf(EX_str, "%s fwd_DM-WB_rs_$%d", EX_str,rs);
		}
		else if( EX_hazard(rs) && !(opcode==0x00 && (funct==SLL||funct==SRL||funct==SRA)) ){
			SrcA = EX_tmp.ALUOut;
			sprintf(EX_str, "%s fwd_EX-DM_rs_$%d", EX_str,rs);
		}
		else{
			SrcA = ID_EX.RegData1;
		}
		//ForwardB (save or not using rt for RegDst)
		if( (ID_EX.MemWrite || ID_EX.RegDst!=0) && MEM_hazard(rt) ){
			WriteData = WB_Result;
			sprintf(EX_str, "%s fwd_DM-WB_rt_$%d", EX_str,rt);
		}
		else if( (ID_EX.MemWrite || ID_EX.RegDst!=0) && EX_hazard(rt) ){
			WriteData = EX_tmp.ALUOut;
			sprintf(EX_str, "%s fwd_EX-DM_rt_$%d", EX_str,rt);
		}
		else{
			WriteData = ID_EX.RegData2;
		}

	}
    //放在外面因為LUI要用
    SrcB = ID_EX.ALUSrc? ID_EX.SignImm : WriteData;

	switch(ID_EX.ALUOp) {
		case ADD:
			EX_tmp.ALUOut = SrcA + SrcB;
			if(has_overflow(EX_tmp.ALUOut, SrcA, SrcB)){
				print_error(NUMBER_OVERFLOW, cycle);
			}
				break;//TODO: overflow
		case SUB:
			EX_tmp.ALUOut = SrcA - SrcB;
			if( has_overflow(EX_tmp.ALUOut,  SrcA, -SrcB) ){
				print_error(NUMBER_OVERFLOW, cycle);
			}
				break;//TODO: overflow
		case AND: EX_tmp.ALUOut = SrcA & SrcB; break;
		case OR:  EX_tmp.ALUOut = SrcA | SrcB; break;
		case XOR: EX_tmp.ALUOut = SrcA ^ SrcB; break;
		case NOR: EX_tmp.ALUOut = ~(SrcA | SrcB); break;
		case NAND: EX_tmp.ALUOut = ~(SrcA & SrcB); break;
		case SLT: EX_tmp.ALUOut = ( (int)SrcA < (int)SrcB ); break;
		case SLL: EX_tmp.ALUOut = SrcB << shamt; break;
		case SRL: EX_tmp.ALUOut = SrcB >> shamt; break;
		case SRA: EX_tmp.ALUOut = (int)SrcB >> shamt; break;
		case LUI: EX_tmp.ALUOut = SrcB << 16; break;
		case JR: case HALT: break;
	}

	EX_tmp.MemRead = ID_EX.MemRead;
	EX_tmp.MemWrite = ID_EX.MemWrite;
	EX_tmp.RegWrite = ID_EX.RegWrite;
	EX_tmp.MemtoReg = ID_EX.MemtoReg;

	EX_tmp.rs = ID_EX.rs;
	EX_tmp.rt = ID_EX.rt;
	EX_tmp.rd = ID_EX.rd;
	EX_tmp.WriteData = WriteData;
	EX_tmp.writeReg = writeReg;
	EX_tmp.instr = instr;
	EX_tmp.PCPlus4 = ID_EX.PCPlus4;
}
void ID_stage(void)
{
	unsigned int instr = IF_ID.instr;
	int opcode = instr>>26;
	int funct = (instr<<26)>>26;
	unsigned int rs, rt, rd, shamt;
	unsigned int Imm;
	rs = (instr<<6)>>27;
	rt = (instr<<11)>>27;
	instr_toString(ID_str,instr);
	if(opcode==HALT) {
		ID_tmp.instr = 0xffffffff;
		haltCnt++;
        return;
    }

	//control value assignment
	if(opcode==0x00){ //R_type

		rd = (instr<<16)>>27;
		shamt  = (instr<<21)>>27;

		ID_tmp.Jump = (funct==JR)? 1:0;
		ID_tmp.Branch = 0;

		ID_tmp.ALUOp = funct;
		ID_tmp.RegDst = 1;
		ID_tmp.ALUSrc = 0;

		ID_tmp.MemRead = 0;
		ID_tmp.MemWrite = 0;

		ID_tmp.RegWrite = (funct==JR) ? 0:1;
		ID_tmp.MemtoReg = 0;
    }
    else if(opcode == J || opcode == JAL) {
        ID_tmp.Jump = 1;
		ID_tmp.Branch = 0;
		ID_tmp.MemRead = 0;
		ID_tmp.MemWrite = 0;
		ID_tmp.RegWrite = (opcode==JAL)? 1:0;
		ID_tmp.MemtoReg = 0;
		ID_tmp.ALUOp = HALT;//不進行運算
		if(opcode==JAL) ID_tmp.RegDst = 2;
    }
    else{ //I_type
		unsigned int uC = (instr<<16)>>16;
		int sC = ((int)(instr<<16))>>16;
		Imm = (opcode==ANDI || opcode==ORI || opcode==NORI) ? uC : sC;

		ID_tmp.Jump = 0;
		ID_tmp.Branch = (opcode==BEQ || opcode==BNE)? 1:0;

		switch(opcode){
			case LUI:  ID_tmp.ALUOp = LUI; break;
			case ANDI: ID_tmp.ALUOp = AND; break;
			case ORI:  ID_tmp.ALUOp = OR;  break;
			case NORI: ID_tmp.ALUOp = NOR; break;
			case SLTI: ID_tmp.ALUOp = SLT; break;
			case BEQ: case BNE: ID_tmp.ALUOp = HALT; break;//不進行運算
			default: ID_tmp.ALUOp = ADD;//預設add
		}
		ID_tmp.RegDst = 0;
		ID_tmp.ALUSrc = (opcode==BEQ || opcode==BNE)? 0:1;

		ID_tmp.MemRead = (opcode==LW || opcode==LH || opcode==LHU || opcode==LB || opcode==LBU)? 1:0;
		ID_tmp.MemWrite = (opcode==SW || opcode==SH || opcode==SB)? 1:0;

		ID_tmp.RegWrite = (opcode==SW || opcode==SH || opcode==SB || opcode==BEQ || opcode==BNE)? 0:1;
		ID_tmp.MemtoReg = (opcode==LW || opcode==LH || opcode==LHU || opcode==LB || opcode==LBU)? 1:0;
    }
	ID_tmp.rs = rs;
	ID_tmp.rt = rt;
	ID_tmp.rd = rd;

    //load-use hazard detection***************************
	if(ID_EX.MemRead && ID_EX.rt!=0) {
		if(opcode==0x00 && funct!=JR){//JR在下面和Branch 一起做
			if(funct==SLL || funct==SRL || funct==SRA){
				if(ID_EX.rt==rt) load_hazard = true;
			}
			else if( ID_EX.rt==rs || ID_EX.rt==rt ){
				load_hazard = true;
			}
		}
		else if(opcode!=J && opcode!=JAL && opcode!=LUI && opcode!=BEQ && opcode!=BNE){
			//I type no LUI no Branch

			if(ID_tmp.MemWrite && (ID_EX.rt==rt||ID_EX.rt==rs) ){ // save 指令rt、rs都要看
                load_hazard = true;
			}
			else if( ID_EX.rt==rs){
				load_hazard = true;
			}
		}
	}

	//JR也要Forward!! 要再ID_EX assign之前判斷
	ID_tmp.RegData1 = reg[rs];
	ID_tmp.RegData2 = reg[rt];
	if(ID_tmp.Branch || (opcode==0x00&&funct==JR) ) { //forward for branch***********
		if( branch_stall_check(rs, rt) ){//TODO: 做branch的DM WB fwd
			load_hazard = true;
		}
		else {
			branch_fwd_check(rs, rt); //check branch/JR fwd
			//change RegData1, RegData2 in the function
		}
	}


	if(ID_tmp.Branch) {  //branch test
		if(opcode==BEQ){
			ID_tmp.BranchTaken = (ID_tmp.RegData1==ID_tmp.RegData2)? 1 : 0;
		}
		else{//BNE
			ID_tmp.BranchTaken = (ID_tmp.RegData1!=ID_tmp.RegData2)? 1 : 0;
		}
	}


	ID_tmp.shamt = shamt;
	ID_tmp.SignImm = Imm;
	ID_tmp.instr = instr;
	ID_tmp.PCPlus4 = IF_ID.PCPlus4;

	//ID_EX = ID_tmp;

	if(load_hazard){ //insert bubble
		strcat(ID_str, " to_be_stalled");
		memset(&ID_tmp,  0, sizeof(PiReg));
	}

}
void IF_stage(int *idx)
{
	int i = *idx;
	sprintf(IF_str, "0x%08X", imem[i]);
    if(imem[i]>>26==HALT) {
        haltCnt++;
    }

    if(load_hazard){ //stall要比HALT先判斷
		strcat(IF_str, " to_be_stalled");
		//prevent update of PC and IF_ID
	}
	else{
        /*if( (imem[i]>>26) == HALT) {//TODO:HALT前面是branch/jump??
            IF_ID.instr = 0xffffffff;
            PC = PC + 4;
            *idx = PC/4;
            return;
        }*/
		IF_ID.instr = imem[i];
		if(ID_tmp.Jump) {
			int opcode = (ID_tmp.instr)>>26;
			unsigned int C = (ID_tmp.instr<<6)>>6;
			switch(opcode){
				case 0x00: //JR
					PC = ID_tmp.RegData1; break;
				case J: case JAL: //JAL >> reg[31]到WB才更改
					PC = (ID_tmp.PCPlus4 & 0xf0000000) | (C*4); break;
				default: puts("Jump error!");
			}
			strcat(IF_str, " to_be_flushed");
			memset(&IF_ID,  0, sizeof(PiReg));//flush
		}
		else if(ID_tmp.Branch && ID_tmp.BranchTaken){
           // printf("instr: %08X\n PC=%X branch=%X\n",imem[PC/4], PC, ID_tmp.PCPlus4+ID_tmp.SignImm*4);
			PC = ID_tmp.PCPlus4 + ID_tmp.SignImm * 4; //不用PC+4因為已經在branch的下一個stage
			strcat(IF_str, " to_be_flushed");
			memset(&IF_ID,  0, sizeof(PiReg));//flush
		}
		else{
			IF_ID.PCPlus4 = PC + 4;
			PC = PC + 4;
		}
		i = PC/4;

		*idx = i;
	}

}
bool branch_stall_check(int rs, int rt)
{
	//load before branch/JR
	if( ID_tmp.Branch && ((ID_EX.MemRead && ID_EX.rt!=0 && (ID_EX.rt==rs||ID_EX.rt==rt)) || (EX_MEM.MemRead && EX_MEM.rt!=0 && (EX_MEM.rt==rs||EX_MEM.rt==rt))) ){ //branch after load
		return true;
	}
	else if(ID_tmp.Jump && ( (ID_EX.MemRead && ID_EX.rt!=0 && ID_EX.rt==rs) || (EX_MEM.MemRead && EX_MEM.rt!=0 && EX_MEM.rt==rs )) ){ //jr after load
		return true;
	}
	else if(ID_tmp.Branch &&  EX_tmp.RegWrite && EX_tmp.writeReg!=0 && (EX_tmp.writeReg==rs || EX_tmp.writeReg==rt) ){
		//branch after non-load instructions
		return true;
	}
	else if(ID_tmp.Jump &&  EX_tmp.RegWrite && EX_tmp.writeReg!=0 && EX_tmp.writeReg==rs){
		//jr after non-load instructions
		return true;
	}

	return false;
}
void branch_fwd_check(int rs, int rt)
{
	if(ID_tmp.Branch){  //Branch

		if(EX_MEM.RegWrite && EX_MEM.writeReg!=0 ){
			if(EX_MEM.writeReg==rs){
				if( (EX_MEM.instr>>26)==JAL)
                    ID_tmp.RegData1 = EX_MEM.PCPlus4;
                else
                    ID_tmp.RegData1 = EX_MEM.ALUOut;
                    sprintf(ID_str, "%s fwd_EX-DM_rs_$%d", ID_str,rs);
            }
			if(EX_MEM.writeReg==rt){
				if( (EX_MEM.instr>>26)==JAL)
                     ID_tmp.RegData2 = EX_MEM.PCPlus4;
				else
                    ID_tmp.RegData2 = EX_MEM.ALUOut;
				sprintf(ID_str, "%s fwd_EX-DM_rt_$%d", ID_str,rt);
			}
		}
	}
	else{//JR
		if(EX_MEM.RegWrite && EX_MEM.writeReg!=0 && EX_MEM.writeReg==rs){
			if( (EX_MEM.instr>>26)==JAL)
                ID_tmp.RegData1 = EX_MEM.PCPlus4;
			else
                ID_tmp.RegData1 = EX_MEM.ALUOut;
			sprintf(ID_str, "%s fwd_EX-DM_rs_$%d", ID_str,rs);
		}
	}
}
bool MEM_hazard(unsigned int rg)
{ //判斷不一定用rd >> 用writeReg
	return (MEM_WB.RegWrite && MEM_WB.writeReg!=0 && !EX_hazard(rg) && MEM_WB.writeReg==rg);
}
bool EX_hazard(unsigned int rg)
{
	return (EX_MEM.RegWrite && EX_MEM.writeReg!=0 && EX_MEM.writeReg==rg);
}
