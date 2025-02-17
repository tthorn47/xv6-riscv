#include "types.h"
#include "param.h"
#include "riscv.h"
#include "spinlock.h"
#include "proc.h"
#include "syscall.h"
#include "defs.h"

#define BUF_PAGES 16
#define MAX_BUFS 10
#define BUF_SIZE (BUF_PAGES * PGSIZE)
#define FOURMEG (4 * 1024 * 1024)
#define NAMELEN 16
//Starting from the top of the virtual address space, go 512MB down
//Should completely rule out interference with kernel addres space (trap page etc.)
#define MAX (MAXVA - FOURMEG*128)
//Each Buffer is stored at a constant location in the memory that descends from MAX
//with FOURMEG of padding between each buffer. This is makes excessively large buffers
//an issue, but an upper limit on size isn't in the spec, so I'm setting one here.
//MAX - (512MB(kernelmem) + 128MB (usermem)) > (10 * ((BUF_SIZE*2) + PGSIZE + FOURMEG))
#define GETADDR(i) PGROUNDDOWN((MAX - (i*((BUF_SIZE*2) + PGSIZE + FOURMEG))))

struct spinlock arrLock;

struct ringbuf* ringbufs[MAX_BUFS];
struct ringbuf {
    int refcount; // 0 for empty
    char name[NAMELEN];
    void* buf[BUF_PAGES];
    void* book;
};

// Creates/Destroys a mapping with ring_buffer named *name*, starting at *mapping*.
// The *flag* value decides whether to create/destroy mapping. 0 is for creating, 1 for destroying.
// Returns: Number of processes mapped to ring_buffer *name*, if call is successful. Otherwise -1.
int ring_call(const char* name, int flag, void** mapping){
    
    if(arrLock.name == 0){
        initlock(&arrLock, "arrLock");
    }

    acquire(&arrLock);

    struct ringbuf* buf = resolve_name(name, flag);
    if(buf == 0){
        printf("Buffer not allocated.\n");
        release(&arrLock);
        return -1;
    }

    if(map_buffer(buf, flag) == 0){
        printf("Can't map into process address space\n");
        release(&arrLock);
        return -1;
    }
    if(!flag){
        uint64 dest = GETADDR(get_index(buf));
        copyout(myproc()->pagetable, (uint64)(mapping), (char*)&dest, sizeof(uint64));
    }
    release(&arrLock);
    return buf->refcount;
}

// Creates/Destroys ring_buffer *name* and allocates associated pages in kernel if a new buffer is created
// flag=0 for allocation. flag=1 for deallocation.
// Returns: ringbuffer pointer or null on error
struct ringbuf* resolve_name(const char* name, int flag){
    int namelen = 0;
    if((namelen = strlen(name)) > 15 || namelen == 0) {
        printf("Name must be less than 16 characters and non-empty\n");
        return 0;
    }

    int i; 
    int first_free = 10;

    for(i = 0; i < MAX_BUFS; i++){
        if(ringbufs[i] == 0){
            if(first_free == 10)
                first_free = i;
            continue;
        }
        if(strncmp(ringbufs[i]->name, name, namelen) == 0){
            if(flag){
                ringbufs[i]->refcount--;
            } else {
                ringbufs[i]->refcount++;
            }
            
            return ringbufs[i];
        }
    }
    
    //No space for new buffer or dealloc one that doens't exist
    if(first_free == 10 || flag){
        if(flag){
            printf("ringbuf: freeing non-existant buffer!\n");
        } else {
            printf("ringbuf: buffer limit reached\n");
        }
        return 0;    
    }

    //map the new buffer at index "first_free"
    if(ringbufs[first_free] == 0){
        if((ringbufs[first_free] = kalloc()) == 0){
            printf("ringbuf: kalloc failed!\n");
            return 0;
        }
        

        int i;
        int stat = 0;
        for (i = 0; i < BUF_PAGES; i++)
        {
            if((ringbufs[first_free]->buf[i] = kalloc()) == 0){
                printf("ringbuf: kalloc failed!\n");
                stat = 1;
                break;
            }
        } 

        if(stat){
            resolve_kill(ringbufs[first_free], i);
            ringbufs[first_free] = 0;     
            return 0;
        }     
        
        
        if((ringbufs[first_free]->book = kalloc()) == 0){
            printf("ringbuf: kalloc failed!\n");
            resolve_kill(ringbufs[first_free], i);
            ringbufs[first_free] = 0;    
            return 0;
        }


        strncpy(ringbufs[first_free]->name, name, namelen);
        ringbufs[first_free]->refcount = 1;
        //printf("%s allocated!\n", ringbufs[first_free]->name);       
        return ringbufs[first_free];
    }
    
    return 0;
}

// Creates a mapping into the processes virtual address space or destroys it depending on flag value.
// If the refcount of the buffer is zero when this method is called, the phymem will be unmapped and 
// the entry will be marked as empty from the side of Kernel bookkeeping.
// Returns: process id if success else 0.
int map_buffer(struct ringbuf* buf, int flag){
      
    int index = get_index(buf);
    struct proc* p = myproc();
    pagetable_t pt = p->pagetable;
    
    //mapping
    if(!flag){

        int i,j, stat;
        for (i = 0; i < BUF_PAGES; i++)
        {
            if((stat = mappages(pt, (GETADDR(index)+(i*PGSIZE)), PGSIZE, (uint64)buf->buf[i], PTE_U | PTE_W | PTE_R | PTE_X)) < 0)
                break;
        }

        //rollback for first mapping if failure occurred
        if(stat < 0){
            printf("ringbuf: mapping to userspace failed!\n");
            alloc_kill(buf, pt, index, i);
            
            return 0;
        }
        
        for (j = 0; j < BUF_PAGES; j++)
        {
            if((stat = mappages(pt, (GETADDR(index)+(j*PGSIZE))+BUF_SIZE, PGSIZE, (uint64)buf->buf[j], PTE_U | PTE_W | PTE_R | PTE_X)) < 0)
                break;
        }

        //rollback if second mapping failed
        if(stat < 0){
            alloc_kill(buf, pt, index, i+j);
            printf("ringbuf: mapping to userspace failed!\n");
            return 0;
        }

        stat = mappages(pt, GETADDR(index)+(BUF_SIZE*2), PGSIZE, (uint64)buf->book, PTE_U | PTE_W | PTE_R | PTE_X);
        
        if(stat < 0){
            printf("ringbuf: mapping to userspace failed!\n");
            alloc_kill(buf, pt, index, BUF_PAGES);          
            return 0;
        }
        p->buffers[index] = buf;

    } else {

        int free = buf->refcount == 0;
        //printf("proc: %d | unmapped %s free = %d\n", p->pid, buf->name, free);
        uvmunmap(pt, GETADDR(index), BUF_PAGES*2+1, 0);
        if(free){
            for(int i = 0; i < BUF_PAGES; i++){
                kfree(buf->buf[i]);
            }
            kfree(buf);
            ringbufs[index] = 0;
            //printf("deallocated\n");
        }    
        p->buffers[index] = 0;     
    }
    
    return p->pid;
}

// Frees the ringbuffer pages if there was an issue kallocing them
void resolve_kill(struct ringbuf* buf, int alloc){
    int i;
    for(i = 0; i < alloc; i++){
        kfree(buf->buf[i]);
    }
    kfree(buf);
}

// Unmaps and frees buffer pages and bookkeeping page 
// This is called both in the event of a mapping error, or by exit()
// if a process exited without closing a buffer properly
void alloc_kill(struct ringbuf* buf, pagetable_t pt, int index, int depth){
    int exit = 0;

    //called by exit(), flush everything
    if(depth == -1){
        printf("ringbuf: warning: process exited without releasing ringbuffer!\n");
        depth = BUF_PAGES*2+1;
        exit = 1;
        //locked is needed externally here, in the standard case, the lock is held by ring_call
        acquire(&arrLock);
    }
    //because this method is called when assigning to the user AS, the mapping might be valid elsewhere
    buf->refcount--;
    uvmunmap(pt, GETADDR(index), depth, 0);
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

// Finds buffer in the array of buffers and returns the index.
int get_index(struct ringbuf* buf){
    int i;
    for(i = 0; i < MAX_BUFS; i++){
        if(buf == ringbufs[i]){
            return i;
        }
    }
    return -1;
}
