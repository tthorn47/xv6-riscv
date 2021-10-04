#include "kernel/types.h"
#include "ringbuffer.h"


int main() {
    char* name = "ThisIsAName";

    int i = ringbuf_attach(name);

    books[i]->write = "This was stored in the ring buffer!";
    printf("%p\n", books[i]->write);
    printf("%s\n", books[i]->write);

    ringbuf_release(i);

    exit(0);
}