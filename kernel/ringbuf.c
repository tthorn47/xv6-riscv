#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "riscv.h"
#include "spinlock.h"
#include "proc.h"
#include "syscall.h"
#include "defs.h"



#define RINGBUF_SIZE 4096
#define MAX_BUFS 10

struct ringbuf* ringbufs[MAX_BUFS];
struct ringbuf {
    int refcount; // 0 for empty
    char name[16];
    void* buf;
    void* book;
};


int ring_call(const char* name, int flag, void** mapping){
    struct ringbuf* buf = resolve_name(name);
    return buf->refcount;
}

struct ringbuf* resolve_name(const char* name){
    int namelen;
    if((namelen = strlen(name)) > 15)
        return 0;
    int i; 
    int first_free = 10;
    for(i = 0; i < MAX_BUFS; i++){
        if(ringbufs[i] == 0 || ringbufs[i]->refcount == 0){
            if(first_free == 10)
                first_free = i;
            continue;
        }
        if(strncmp(ringbufs[i]->name, name, namelen) == 0){
            ringbufs[i]->refcount++;
            printf("Ringbuffer found!\n");
            return ringbufs[i];
        }
    }
    //No space for new buffer
    if(first_free == 10){
        return 0;
    }
    //open space is unallocated
    if(ringbufs[first_free] == 0){
        ringbufs[first_free] = kalloc();
        ringbufs[first_free]->refcount = 1;
        ringbufs[first_free]->buf = kalloc();
        strncpy(ringbufs[first_free]->name, name, namelen);
        ringbufs[first_free]->book = kalloc();

        printf("Ringbuffer allocated!\n");

        return ringbufs[first_free];
    }

    return 0;
}