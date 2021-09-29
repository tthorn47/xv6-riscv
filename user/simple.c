#include "kernel/types.h"
#include "user.h"

int main(){
    const char* name = "ThisIsAName";
    void* p = malloc(sizeof(void*));

    ringbuf(name,0,&p);

    p = "This was stored in the ring buffer!";
    printf("%s\n",p);
    

    const char* o_name = "Diffname";
    void* x = malloc(sizeof(void*));
    ringbuf(o_name,0,&x);
    x = "This was stored in a different ring buffer!";
    printf("%s\n",x);
    ringbuf(o_name,1,&x);
    ringbuf(name,1,&p);

    ringbuf(name,0,&p);

    p = "This was stored in a ring buffer with the same name as the first ring buffer!";
    printf("%s\n",p);
    ringbuf(name,1,&p);
    free(p);
    free(x);
    exit(0);
}