#include "user.h"
#include "kernel/types.h"

int main(){
    for (int i = 0; i < 5; i++)
    {
        uint64 cycles;
        if(read(2, &cycles, 8) < 0){
            printf("Cycle read failed!");
            exit(-1);
        }
        printf("Cycle count : %d", cycles);
    }
    exit(0);
}