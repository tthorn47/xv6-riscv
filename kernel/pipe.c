#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "param.h"
#include "spinlock.h"
#include "proc.h"
#include "fs.h"
#include "sleeplock.h"
#include "file.h"

#define PIPESIZE 1024

struct pipe {
  struct spinlock lock;
  char data[PIPESIZE];
  uint nread;     // number of bytes read
  uint nwrite;    // number of bytes written
  int readopen;   // read fd is still open
  int writeopen;  // write fd is still open
};

int
pipealloc(struct file **f0, struct file **f1)
{
  struct pipe *pi;

  pi = 0;
  *f0 = *f1 = 0;
  if((*f0 = filealloc()) == 0 || (*f1 = filealloc()) == 0)
    goto bad;
  if((pi = (struct pipe*)kalloc()) == 0)
    goto bad;
  pi->readopen = 1;
  pi->writeopen = 1;
  pi->nwrite = 0;
  pi->nread = 0;
  initlock(&pi->lock, "pipe");
  (*f0)->type = FD_PIPE;
  (*f0)->readable = 1;
  (*f0)->writable = 0;
  (*f0)->pipe = pi;
  (*f1)->type = FD_PIPE;
  (*f1)->readable = 0;
  (*f1)->writable = 1;
  (*f1)->pipe = pi;
  return 0;

 bad:
  if(pi)
    kfree((char*)pi);
  if(*f0)
    fileclose(*f0);
  if(*f1)
    fileclose(*f1);
  return -1;
}

void
pipeclose(struct pipe *pi, int writable)
{
  acquire(&pi->lock);
  if(writable){
    pi->writeopen = 0;
    wakeup(&pi->nread);
  } else {
    pi->readopen = 0;
    wakeup(&pi->nwrite);
  }
  if(pi->readopen == 0 && pi->writeopen == 0){
    release(&pi->lock);
    kfree((char*)pi);
  } else
    release(&pi->lock);
}

int
pipewrite(struct pipe *pi, uint64 addr, int n)
{
  int i = 0;
  struct proc *pr = myproc();
  //char buf[PIPESIZE];
  acquire(&pi->lock);
  while(i < n){
    if(pi->readopen == 0 || pr->killed){
      release(&pi->lock);
      return -1;
    }
    if(pi->nwrite == pi->nread + PIPESIZE){ //DOC: pipewrite-full
      wakeup(&pi->nread);
      sleep(&pi->nwrite, &pi->lock);
    } else {
      int k = 0;
      //pi->nwrite % PIPESIZE = write's position in buffer
      //pi->nwrite - (pi->nread + PIPESIZE) = how much buffer space isn't filled with unread data
      if(pi->nwrite % PIPESIZE < n-i && pi->nwrite % PIPESIZE < (pi->nread + PIPESIZE) - pi->nwrite && pi->nwrite % PIPESIZE != 0){
        k = pi->nwrite % PIPESIZE;
        //printf("if k = %d", k);
      } else if ((pi->nread + PIPESIZE) - pi->nwrite < n-i){     
        k = (pi->nread + PIPESIZE) - pi->nwrite;
        //printf("elif k = %d", k);
      } else {
        k = n-i;
        //printf("el k = %d", k);
      }
      // if(k == 1024){
      //   printf("k = %d\n", k);
      // }
      if(copyin(pr->pagetable, &pi->data[pi->nwrite % PIPESIZE], addr+i, k) == -1)
        break;
       pi->nwrite+=k;
      i+=k;
    }
  }
  //bad:
  wakeup(&pi->nread);
  release(&pi->lock);

  return i;
}

int
piperead(struct pipe *pi, uint64 addr, int n)
{
  int i = 0;
  struct proc *pr = myproc();
  

  acquire(&pi->lock);
  while(pi->nread == pi->nwrite && pi->writeopen){  //DOC: pipe-empty
    if(pr->killed){
      release(&pi->lock);
      return -1;
    }
    sleep(&pi->nread, &pi->lock); //DOC: piperead-sleep
  }
    //Check for the smallest of three values, commit that value to copyout
    //1. The amount the user requests: n
    //2. The amount of unread data in the pipe: (pi->nwrite-pi->nread)
    //3. The pipes position relative to the end (a wrap around is needed): pi->nread % PIPESIZE
    if(pi->nread % PIPESIZE < n && pi->nread % PIPESIZE < (pi->nwrite-pi->nread) && pi->nread % PIPESIZE != 0){
      i = pi->nread % PIPESIZE;
    } else if ((pi->nwrite - pi->nread) < n){
      i = pi->nwrite - pi->nread;
    } else {
      i = n;
    }
    //printf("Provided Addr = %p\nBuffer Position = %p\n", addr, &pi->data[pi->nread % PIPESIZE]);
    //increment nread and return the value of bytes copied only if the copyout returns zero
    if(copyout(pr->pagetable, addr, &pi->data[pi->nread % PIPESIZE], i) == -1){
      printf("read failed\n");
      i = 0;
    } else {
      pi->nread+=i;
    }
  //}
  wakeup(&pi->nwrite);  //DOC: piperead-wakeup
  release(&pi->lock);
  return i;
}
