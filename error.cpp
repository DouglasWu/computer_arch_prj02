#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "parameter.h"
#include "error.h"

bool is_nop(unsigned int instr)
{
    int funct = ( instr << 26 ) >> 26;
    unsigned int rs = (instr<<6) >>27;
    unsigned int rt = (instr<<11)>>27;
    unsigned int rd = (instr<<16)>>27;
    unsigned int C  = (instr<<21)>>27;

    return funct==SLL && rt==0 && rd==0 && C==0;
}
void print_error(int type, int cyc)
{
    switch(type){
        case WRITE_ZERO:
            fprintf( error_dump , "In cycle %d: Write $0 Error\n", cyc);
            break;
        case NUMBER_OVERFLOW:
            fprintf(error_dump , "In cycle %d: Number Overflow\n", cyc);
            break;
        case ADDRESS_OVERFLOW:
            fprintf(error_dump , "In cycle %d: Address Overflow\n", cyc);
            break;
        case MISALIGNMENT:
            fprintf(error_dump , "In cycle %d: Misalignment Error\n", cyc);
            break;
    }
}
bool has_overflow(int c, int a, int b)
{
    if( (a>>31)==(b>>31) && (a>>31)!=(c>>31) )
        return true;
    return false;
}

void check_errors(int rt, int addr, int reg_s, int sC, int op)
{
    int unit;
    if(op==LW || op==SW) unit = 4;
    else if(op==LH || op==LHU || op==SH) unit = 2;
    else unit = 1;

    if(rt==0)
        print_error(WRITE_ZERO, cycle);
    if( has_overflow(addr, reg_s, sC) )
        print_error(NUMBER_OVERFLOW, cycle);
    if(addr >= MEM_SIZE-3 || addr<0){
        bool has_error = false;
        if(addr<0 || unit==4 || (unit==2 && addr >= MEM_SIZE-1) || (unit==1 && addr>=MEM_SIZE))
            has_error = true;
        if(has_error){
            print_error(ADDRESS_OVERFLOW, cycle);
            error_halt = true;
        }
    }

    if(addr%unit!=0){
        print_error(MISALIGNMENT, cycle);
        error_halt = true;
    }
}


