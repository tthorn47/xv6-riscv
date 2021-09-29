#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "riscv.h"
#include "spinlock.h"
#include "proc.h"
#include "syscall.h"
#include "defs.h"

#define BUF_PAGES 16
#define PAGE 4096
#define MAX_BUFS 10
#define BUF_SIZE BUF_PAGES * PAGE
#define FOURMEG 4194304

#define MAX (92233720368 - 16384)
#define GETADDR(i) (MAX - (i*(BUF_SIZE + 4096 + FOURMEG)))
#define GETDUPADDR(i) (MAX - ((i*(BUF_SIZE + 4096 + FOURMEG)) - (BUF_SIZE+4096)))

struct spinlock arrLock;

struct ringbuf* ringbufs[MAX_BUFS];
struct ringbuf {
    int refcount; // 0 for empty
    char name[16];
    void* buf[BUF_PAGES];
    void* book;
};


int ring_call(const char* name, int flag, void** mapping){
    if(arrLock.name == 0){
        initlock(&arrLock, "arrLock");
    }
    struct ringbuf* buf = resolve_name(name, flag);
    buf_alloc(buf, flag);
    return buf->refcount;
}

struct ringbuf* resolve_name(const char* name, int flag){
    //Name must be less than 16 chars and not empty
    int namelen;
    if((namelen = strlen(name)) > 15 || namelen == 0)
        return 0;

    int i; 
    int first_free = 10;
    acquire(&arrLock);
    for(i = 0; i < MAX_BUFS; i++){
        if(ringbufs[i] == 0){
            if(first_free == 10)
                first_free = i;
            continue;
        }
        if(strncmp(ringbufs[i]->name, name, namelen) == 0){
            if(!flag){
                ringbufs[i]->refcount++;
            } else {
                ringbufs[i]->refcount--;
            }
            printf("Ringbuffer found!\n");
            release(&arrLock);
            return ringbufs[i];
        }
    }
    release(&arrLock);
    //No space for new buffer
    if(first_free == 10){
        return 0;
    }
    acquire(&arrLock);
    //open space is unallocated
    if(ringbufs[first_free] == 0){
        ringbufs[first_free] = kalloc();
        ringbufs[first_free]->refcount = 1;

        int i;
        for (i = 0; i < BUF_PAGES; i++)
        {
            ringbufs[first_free]->buf[i] = kalloc();
        }      
        
        strncpy(ringbufs[first_free]->name, name, namelen);
        ringbufs[first_free]->book = kalloc();

        printf("Ringbuffer allocated!\n");
        release(&arrLock);
        return ringbufs[first_free];
    }
    release(&arrLock);
    return 0;
}

int buf_alloc(struct ringbuf* buf, int flag){
    struct proc* p = myproc();
    pagetable_t pt = p->pagetable;
    int index = get_index(buf);
    acquire(&arrLock);
    if(flag == 0){
        int i;
        for (i = 0; i < BUF_PAGES; i++)
        {
            mappages(pt, PGROUNDDOWN(GETADDR(index)+(i*PAGE)), PAGE, (uint64)buf->buf[i], PTE_U | PTE_W | PTE_R | PTE_X);
        }
        
        for (i = 0; i < BUF_PAGES; i++)
        {
            mappages(pt, PGROUNDDOWN(GETADDR(index)+(i*PAGE))+BUF_SIZE, PAGE, (uint64)buf->buf[i], PTE_U | PTE_W | PTE_R | PTE_X);
        }
        mappages(pt, PGROUNDDOWN(GETADDR(index))+(BUF_SIZE*2), 4096, (uint64)buf->book, PTE_U | PTE_W | PTE_R | PTE_X);
    } else {
        int free = buf->refcount == 0;
        uvmunmap(pt, PGROUNDDOWN(GETADDR(index)), BUF_PAGES*2+1, free);

        if(free)
            kfree(buf);
    }
    release(&arrLock);
    return p->pid;
}

int get_index(struct ringbuf* buf){
    int i;
    acquire(&arrLock);
    for(i = 0; i < MAX_BUFS; i++){
        if(buf == ringbufs[i]){
            release(&arrLock);
            return i;
        }
    }
    release(&arrLock);
    return -1;
}