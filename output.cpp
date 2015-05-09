#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "parameter.h"
#include "output.h"

bool is_nop(unsigned int instr)
{
    int opcode = (instr>>26);
    int funct = ( instr << 26 ) >> 26;
    unsigned int rt = (instr<<11)>>27;
    unsigned int rd = (instr<<16)>>27;
    unsigned int C  = (instr<<21)>>27;

    return opcode==0x00 && funct==SLL && rt==0 && rd==0 && C==0;
}
void instr_toString(char* str, unsigned int instr)
{
	int opcode = instr>>26;
    int funct = ( instr << 26 ) >> 26;


    if(opcode==0x00){
		if(is_nop(instr)){
			sprintf(str,"NOP");
		}
		else {
			switch(funct){
				case ADD: sprintf(str,"ADD"); break;
				case SUB: sprintf(str,"SUB"); break;
				case AND: sprintf(str,"AND"); break;
				case OR:  sprintf(str,"OR"); break;
				case XOR:  sprintf(str,"XOR");  break;
                case NOR:  sprintf(str,"NOR");  break;
                case NAND: sprintf(str,"NAND"); break;
                case SLT:  sprintf(str,"SLT");  break;
                case SLL:  sprintf(str,"SLL");  break;
                case SRL:  sprintf(str,"SRL");  break;
                case SRA:  sprintf(str,"SRA");  break;
                case JR:   sprintf(str,"JR");   break;
				default:  puts("in instr_toString(): No such R-type instr!");
			}
		}
	}
	else if(opcode==J || opcode==JAL){
		if(opcode==J) sprintf(str,"J");
		else sprintf(str,"JAL");
	}
	else{
		switch(opcode){
			case ADDI: sprintf(str,"ADDI"); break;
            case LW:   sprintf(str,"LW"  ); break;
            case LH:   sprintf(str,"LH"  ); break;
            case LHU:  sprintf(str,"LHU" ); break;
            case LB:   sprintf(str,"LB"  ); break;
            case LBU:  sprintf(str,"LBU" ); break;
            case SW:   sprintf(str,"SW"  ); break;
            case SH:   sprintf(str,"SH"  ); break;
            case SB:   sprintf(str,"SB"  ); break;
            case LUI:  sprintf(str,"LUI" ); break;
            case ANDI: sprintf(str,"ANDI"); break;
            case ORI:  sprintf(str,"ORI" ); break;
            case NORI: sprintf(str,"NORI"); break;
            case SLTI: sprintf(str,"SLTI"); break;
            case BEQ:  sprintf(str,"BEQ" ); break;
            case BNE:  sprintf(str,"BNE" ); break;
            case HALT: sprintf(str,"HALT"); break;
			default: puts("in instr_toString(): No such I-type instr!");
			printf("   instr: 0x%08X\n\n",instr);
		}
	}
}
void print_cycle(void)
{
    fprintf(snapshot, "cycle %d\n",cycle);
    for(int i=0; i<32; i++)
        fprintf(snapshot, "$%02d: 0x%08X\n",i,reg[i]);
    fprintf(snapshot,"PC: 0x%08X\n",PC);

}
void print_pipeline_stage(void)
{
	fprintf(snapshot,"IF: %s\n",IF_str);
	fprintf(snapshot,"ID: %s\n",ID_str);
	fprintf(snapshot,"EX: %s\n",EX_str);
	fprintf(snapshot,"DM: %s\n",DM_str);
	fprintf(snapshot,"WB: %s\n\n\n",WB_str);
}
