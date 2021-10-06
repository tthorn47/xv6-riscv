#include "kernel/types.h"
#include "user.h"
#include "ringbuffer.h"

#define BUF_PAGES 16
#define PAGE 4096
#define BUF_SIZE (BUF_PAGES * PAGE)
#define MAX_BUFS 10

int main(){
    int a = ringbuf_attach("a");
    int b = ringbuf_attach("b");
    int c = ringbuf_attach("c");
    int d = ringbuf_attach("d");
    int e = ringbuf_attach("e");
    int f = ringbuf_attach("f");
    int g = ringbuf_attach("g");
    int h = ringbuf_attach("h");
    int i = ringbuf_attach("i");
    int j = ringbuf_attach("j");
    int k = ringbuf_attach("k");

        
    exit(a+b+c+d+e+f+g+h+i+j+k);
}