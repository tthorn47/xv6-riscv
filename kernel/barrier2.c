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
static int sleepers = 0;
static struct spinlock bar_lock;
static int barrier_read(int user_src, uint64 src, int n){
    if(n != 4)
        return -1;
    
    acquire(&bar_lock);
    struct proc* p = myproc();

    ++sleepers;
    if(either_copyout(user_src, src, &sleepers, 4) < 0){
        --sleepers;
        release(&bar_lock);
        return -1;
    }

    acquire(&p->lock);
    myproc()->state = SLEEPING;
    release(&p->lock);
    
    sleep(&devsw[BARRIER2], &bar_lock);
    release(&bar_lock);
    return 4;
}

static int barrier_write(int user_src, uint64 src, int n){
    if(n != 4)
        return -1;
    
    acquire(&bar_lock);
    wakeup(&devsw[BARRIER2]);
    release(&bar_lock);
    return 4;
}

void barrier2_init(){
    initlock(&bar_lock, "bar_lock");
    devsw[BARRIER2].read = barrier_read;
    devsw[BARRIER2].write = barrier_write;
}