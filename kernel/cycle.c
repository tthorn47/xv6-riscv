#include <stdarg.h>

#include "types.h"
#include "param.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "fs.h"
#include "file.h"
#include "memlayout.h"
#include "riscv.h"
#include "defs.h"
#include "proc.h"

unsigned long rdcycle(void) {
  unsigned long dst;
  asm volatile ("csrrs %0, 0xc00, x0" : "=r" (dst));
  return dst;
}

int cycle_read(int user_src, uint64 src, int n){
    if(n != 8){
        return -1;
    }

    uint64 count = rdcycle();

    either_copyout(1, src, &count, 8);

    return 8;
}

int cycle_write(int user_src, uint64 src, int n){
    panic("Attempting to write to the cycle counter!\n");
}

void cycle_init(void){
    printf("cycle init\n");
    devsw[CYCLE].read = cycle_read;
    devsw[CYCLE].write = cycle_write;
}
