#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "parameter.h"
#include "error.h"
#include "instruction.h"

void R_type(unsigned int instr)
{
    int funct = ( instr << 26 ) >> 26;
    unsigned int rs = (instr<<6) >>27;
    unsigned int rt = (instr<<11)>>27;
    unsigned int rd = (instr<<16)>>27;
    unsigned int C  = (instr<<21)>>27;

    if(funct!=JR && rd==0 && !is_nop(instr)){
        print_error(WRITE_ZERO, cycle);
        //return;
    }

    switch(funct){
    case ADD:
        if( has_overflow(reg[rs]+reg[rt], reg[rs], reg[rt]) )
            print_error(NUMBER_OVERFLOW, cycle);
        reg[rd] = reg[rs] + reg[rt];
        if(rd==0) reg[rd] = 0;
        break;
    case SUB:
        if( has_overflow(reg[rs]-reg[rt], reg[rs], -reg[rt]) )
            print_error(NUMBER_OVERFLOW, cycle);
        reg[rd] = reg[rs] - reg[rt];
        if(rd==0) reg[rd] = 0;
        break;
    case AND:
        reg[rd] = reg[rs] & reg[rt];
        if(rd==0) reg[rd] = 0;
        break;
    case OR:
        reg[rd] = reg[rs] | reg[rt];
        if(rd==0) reg[rd] = 0;
        break;
    case XOR:
        reg[rd] = reg[rs] ^ reg[rt];
        if(rd==0) reg[rd] = 0;
        break;
    case NOR:
        reg[rd] = ~(reg[rs] | reg[rt]);
        if(rd==0) reg[rd] = 0;
        break;
    case NAND:
        reg[rd] = ~(reg[rs] & reg[rt]);
        if(rd==0) reg[rd] = 0;
        break;
    case SLT:
        reg[rd] = ( (int)reg[rs] < (int)reg[rt] );  //signed comparison
        if(rd==0) reg[rd] = 0;
        break;
    case SLL:
        reg[rd] = reg[rt] << C;
        if(rd==0) reg[rd] = 0;
        break;
    case SRL:
        reg[rd] = reg[rt] >> C;
        if(rd==0) reg[rd] = 0;
        break;
    case SRA:
        reg[rd] = (int)reg[rt] >> C;
        if(rd==0) reg[rd] = 0;
        break;
    case JR:
        PC = reg[rs];
        pcChanged = true;
        break;
    default:
        puts("decode fail!");
    }

    //printf("%d %d %d\n",rs, rt, rd, C);
}
void I_type(unsigned int instr)
{
    int opcode = instr >> 26;
    unsigned int rs = (instr<<6) >>27;
    unsigned int rt = (instr<<11)>>27;
    unsigned int uC  = (instr<<16)>>16;
    int sC = ((int)(instr<<16))>>16;
    unsigned int masks[4] = {0x00ffffff, 0xff00ffff, 0xffff00ff, 0xffffff00};
    int shift, save, tmp;
    unsigned int tmpu;
    bool overflow = false;
    unsigned int addr = reg[rs] + sC;/**handle memory addr overflow*/

    switch(opcode){
    case ADDI:
        if(rt==0){
            print_error(WRITE_ZERO, cycle);
        }
        if( has_overflow(reg[rs]+sC, reg[rs], sC) )
             print_error(NUMBER_OVERFLOW, cycle);
        reg[rt] = reg[rs] + sC;
        if(rt==0) reg[rt] = 0;
        break;

    case LW:
        check_errors(rt, addr, reg[rs], sC, LW);
        if(error_halt) return;
        reg[rt] = dmem[addr/4];
        if(rt==0) reg[rt]=0;
        break;
    case LH: //signed
        check_errors(rt, addr, reg[rs], sC, LH);
        if(error_halt) return;
        tmp = dmem[addr/4];
        reg[rt] = addr%4==0 ? tmp>>16 : (tmp<<16)>>16;
        if(rt==0) reg[rt]=0;
        break;
    case LHU:
        check_errors(rt, addr, reg[rs], sC, LHU);
        if(error_halt) return;
        tmpu = dmem[addr/4];
        reg[rt] = addr%4==0 ? tmpu>>16 : (tmpu<<16)>>16;
        //printf("lhu $%s, %d($%s)\n",regName[rt], sC, regName[rs]); break;
        if(rt==0) reg[rt]=0;
        break;
    case LB:
        check_errors(rt, addr, reg[rs], sC, LB);
        if(error_halt) return;
        tmp = dmem[addr/4];
        shift = (addr%4)*8;
        reg[rt] = (tmp << shift) >> 24;
        if(rt==0) reg[rt]=0;
        break;
    case LBU:
        check_errors(rt, addr, reg[rs], sC, LBU);
        if(error_halt) return;
        tmpu = dmem[addr/4];
        shift = (addr%4)*8;
        reg[rt] = (tmpu << shift) >> 24;
        if(rt==0) reg[rt]=0;
        break;
    case SW:
        check_errors(-1, addr, reg[rs], sC, SW);//SW不會有write to zero reg的問題所以rt設-1
        if(error_halt) return;
        dmem[addr/4] = reg[rt];
        break;
    case SH:
        check_errors(-1, addr, reg[rs], sC, SH);
        if(error_halt) return;
        tmpu = dmem[addr/4];
        save = reg[rt] & 0x0000ffff;
        if(addr%4==0){
            dmem[addr/4] = (tmpu&0x0000ffff) + (save<<16);
        }else{
            dmem[addr/4] = ((tmpu>>16)<<16) + save;
        }
        break;
    case SB:
        check_errors(-1, addr, reg[rs], sC, SB);
        if(error_halt) return;
        tmpu = dmem[addr/4];
        save = reg[rt] & 0x000000ff;
        shift = 24 - (addr%4)*8;
        dmem[addr/4] = (tmpu & masks[addr%4]) + (save<<shift);
        break;

    case LUI:
        if(rt==0){
            print_error(WRITE_ZERO, cycle);
            return;
        }
        reg[rt] = uC << 16;
        break;
    case ANDI:
        if(rt==0){
            print_error(WRITE_ZERO, cycle);
            return;
        }
        reg[rt] = reg[rs] & uC;
        break;
    case ORI:
        if(rt==0){
            print_error(WRITE_ZERO, cycle);
            return;
        }
        reg[rt] = reg[rs] | uC;
        break;
    case NORI:
        if(rt==0){
            print_error(WRITE_ZERO, cycle);
            return;
        }
        reg[rt] = ~(reg[rs] | uC);
        break;
    case SLTI:
        if(rt==0){
            print_error(WRITE_ZERO, cycle);
            return;
        }
        reg[rt] = ( (int)reg[rs] < sC );
        break;

    case BEQ:
        if(reg[rs]==reg[rt]){
            if( has_overflow( PC+4+sC*4, PC+4, sC*4) )
                print_error(NUMBER_OVERFLOW, cycle);
            if( PC+4+sC*4 >= MEM_SIZE){
                print_error(ADDRESS_OVERFLOW, cycle);
                error_halt  = true;
            }
            PC = (PC+4) + sC*4;
            pcChanged = true;
        }

        break;
    case BNE:
        if(reg[rs]!=reg[rt]){
            if( has_overflow( PC+4+sC*4, PC+4, sC*4) )
                print_error(NUMBER_OVERFLOW, cycle);
            if( PC+4+sC*4 >= MEM_SIZE){
                print_error(ADDRESS_OVERFLOW, cycle);
                error_halt  = true;
            }
            PC = (PC+4) + sC*4;
            pcChanged = true;
        }

        break;
    default:
        puts("decode fail!");
    }
}
void J_type(unsigned int instr)
{
    int opcode = instr >> 26;
    unsigned int C = (instr<<6)>>6;


    if(opcode == JAL){
        reg[31] = PC + 4;
    }
    PC = ((PC+4) & 0xf0000000) | (C*4);
    pcChanged = true;
}

