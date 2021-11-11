#include "kernel/types.h"
#include "user.h"

int main(){
    for (int i = 0; i < 5; i++)
    {
        uint64 cycles;
        if(read(3, &cycles, 8) < 0){
            printf("Cycle read failed!\n");
            exit(-1);
        }
        printf("Cycle count as 64bit hex value: %p\n", cycles);
    }
    exit(0);
}