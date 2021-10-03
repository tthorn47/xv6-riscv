#include "kernel/types.h"
#include "ringbuffer.h"


int main() {
    char* name = "ThisIsAName";

    int i = ringbuf_attach(name);

    user_ringbufs[i]->buf = "This was stored in the ring buffer!";
    printf("%p\n", user_ringbufs[i]->buf);
    printf("%s\n", user_ringbufs[i]->buf);

    ringbuf_release(name, i);

    exit(0);
}