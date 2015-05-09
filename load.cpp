#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "parameter.h"
#include "load.h"

void init(void)
{
    for(int i=0; i<32; i++)
        reg[i] = ZERO;
    reg[29] = sp;

	memset(&IF_ID,  0, sizeof(PiReg));
	memset(&ID_EX,  0, sizeof(PiReg));
	memset(&EX_MEM, 0, sizeof(PiReg));
	memset(&MEM_WB, 0, sizeof(PiReg));

    snapshot = fopen("snapshot.rpt", "w");
    error_dump = fopen("error_dump.rpt", "w");
}
unsigned int convert(unsigned char bytes[])
{
    return bytes[3] | (bytes[2]<<8) | (bytes[1]<<16) | (bytes[0]<<24);
}
bool load_image(void)
{
    FILE *fi = fopen("iimage.bin", "rb");
    FILE *fd = fopen("dimage.bin", "rb");
    if(!fi || !fd) return false;

    for(int i = 0; i<MEM_SIZE/4; i++)
        imem[i] = dmem[i] = ZERO;

    unsigned char bytes[4];
    int i = 0;
    while( fread(bytes, 4, 1, fi) != 0 ){
        if(i==0){
            PC = convert(bytes);
        }
        else if(i==1){
            iSize = convert(bytes);
        }
        else{
            if(i-2 >= iSize) break;
            imem[i-2 + PC/4] = convert(bytes);
           // printf("%08x \n", imem[i-2]);
        }

        i++;
    }
   // puts("");

    i = 0;
    while( fread(bytes, 4, 1, fd) != 0 ){
        if(i==0){
            sp = convert(bytes);
        }
        else if(i==1){
            dSize = convert(bytes);
        }
        else{
            if(i-2 >= dSize) break;
            dmem[i-2] = convert(bytes);
            //printf("%08x \n", dmem[i-2]);
        }
        i++;
    }
    fclose(fi);
    fclose(fd);
    return true;
}
