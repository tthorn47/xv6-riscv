#include "kernel/types.h"
#include "user.h"
#include "ringbuffer.h"
int main(){
        int i = ringbuf_attach("name");
        int j = ringbuf_attach("name2");
        //printf("%d = i\n", i);
        ringbuf_release(i);
        ringbuf_release(j);
    exit(0);
}