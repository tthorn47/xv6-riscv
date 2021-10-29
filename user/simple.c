#include "kernel/types.h"
#include "user.h"
#include "ringbuffer.h"

#define BUF_PAGES 16
#define PAGE 4096
#define BUF_SIZE (BUF_PAGES * PAGE)
#define MAX_BUFS 10

int main(){
 fork();
 fork();
 fork();
 fork();
 fork();
 fork();
 fork();
 fork();
 exit(0);
}