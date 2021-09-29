#include "kernel/types.h"
#include "user.h"

int main(){
    const char* name = "ThisIsAName";
    void* p = malloc(sizeof(void*));

    ringbuf(name,0,&p);

    p = "This was stored in the ring buffer!";
    printf("%s\n",p);
    ringbuf(name,1,&p);
    free(p);
    exit(0);
}