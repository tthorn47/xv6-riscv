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

#define MAX (MAXVA - FOURMEG*128)
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
    if(buf == 0){
        printf("Buffer not allocated, kalloc failure\n");
        return -1;
    }

    if(buf_alloc(buf, flag) == 0){
        printf("Can't map into processes address space");
        return -1;
    }
    if(!flag){
        uint64 dest = PGROUNDDOWN(GETADDR(get_index(buf)));
        copyout(myproc()->pagetable, (uint64)(mapping), (char*)&dest, sizeof(uint64));
    }
    //*mapping = (void*));
    return buf->refcount;
}

struct ringbuf* resolve_name(const char* name, int flag){
    acquire(&arrLock);
    //Name must be less than 16 chars and not empty
    int namelen;
    if((namelen = strlen(name)) > 15 || namelen == 0)
        return 0;

    int i; 
    int first_free = 10;
    
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
            printf("%s found!\n", ringbufs[i]->name);
            release(&arrLock);
            return ringbufs[i];
        }
    }
    
    //No space for new buffer
    if(first_free == 10 || flag){
        return 0;
        release(&arrLock);
    }
    //open space is unallocated
    if(ringbufs[first_free] == 0){
        if((ringbufs[first_free] = kalloc()) == 0){
            release(&arrLock);
            return 0;
        }
        ringbufs[first_free]->refcount = 1;

        int i;
        int stat = 0;
        for (i = 0; i < BUF_PAGES; i++)
        {
            if((ringbufs[first_free]->buf[i] = kalloc()) == 0){
                stat = 1;
                break;
            }             
        } 

        if(stat){
            resolve_kill(ringbufs[first_free], i);
            ringbufs[first_free] = 0;
            release(&arrLock);
            return 0;
        }     
        
        
        if((ringbufs[first_free]->book = kalloc()) == 0){
            resolve_kill(ringbufs[first_free], i);
            ringbufs[first_free] = 0;
            release(&arrLock);
            return 0;
        }


        strncpy(ringbufs[first_free]->name, name, namelen);
        printf("%s allocated!\n", ringbufs[first_free]->name);
        release(&arrLock);
        return ringbufs[first_free];
    }
    release(&arrLock);
    return 0;
}

int buf_alloc(struct ringbuf* buf, int flag){
    
    acquire(&arrLock);
    int index = get_index(buf);
    struct proc* p = myproc();
    pagetable_t pt = p->pagetable;
    
    

    if(flag == 0){

        int i,j, stat;
        for (i = 0; i < BUF_PAGES; i++)
        {
            if((stat = mappages(pt, PGROUNDDOWN(GETADDR(index)+(i*PAGE)), PAGE, (uint64)buf->buf[i], PTE_U | PTE_W | PTE_R | PTE_X)) < 0)
                break;
        }

        //rollback for first mapping if failure occurred
        if(stat < 0){
            alloc_kill(buf, pt, index, i);
            release(&arrLock);
            return 0;
        }
        
        for (j = 0; j < BUF_PAGES; j++)
        {
            if((stat = mappages(pt, PGROUNDDOWN(GETADDR(index)+(j*PAGE))+BUF_SIZE, PAGE, (uint64)buf->buf[j], PTE_U | PTE_W | PTE_R | PTE_X)) < 0)
                break;
        }

        if(stat < 0){
            alloc_kill(buf, pt, index, i+j);
            release(&arrLock);
            return 0;
        }

        stat = mappages(pt, PGROUNDDOWN(GETADDR(index))+(BUF_SIZE*2), 4096, (uint64)buf->book, PTE_U | PTE_W | PTE_R | PTE_X);
        
        if(stat < 0){
            alloc_kill(buf, pt, index, BUF_PAGES);
            release(&arrLock);
            return 0;
        }
        p->buffers[index] = buf;
    } else {
        int free = buf->refcount == 0;
        uvmunmap(pt, PGROUNDDOWN(GETADDR(index)), BUF_PAGES*2+1, 0);
        

        if(free){
            for(int i = 0; i < BUF_PAGES; i++){
                kfree(buf->buf[i]);
            }
            kfree(buf);
            ringbufs[index] = 0;
        }
        
        p->buffers[index] = 0;
    }
    
    release(&arrLock);
    return p->pid;
}

void resolve_kill(struct ringbuf* buf, int alloc){
    int i;
    for(i = 0; i < alloc; i++){
        kfree(buf->buf[i]);
    }
    kfree(buf);
}


void alloc_kill(struct ringbuf* buf, pagetable_t pt, int index, int depth){
    int exit = 0;
    if(depth == -1){
        depth = BUF_PAGES*2+1;
        exit = 1;
        acquire(&arrLock);
    }
    buf->refcount--;
    //if called by the kernel for an exiting process, set the depth to max
    
    uvmunmap(pt, PGROUNDDOWN(GETADDR(index)), depth, 0);
    //free the physmem as a separate op to make logic simpler.
    if(buf->refcount == 0){
        int i;
        for(i = 0; i < BUF_PAGES; i++)
            kfree(buf->buf[i]);

        kfree(buf->book);
        kfree(buf);
        ringbufs[index] = 0;
    }
    if(exit){
        release(&arrLock);
    }
}

int get_index(struct ringbuf* buf){
    int i;
    for(i = 0; i < MAX_BUFS; i++){
        if(buf == ringbufs[i]){
            return i;
        }
    }
    return -1;
}
