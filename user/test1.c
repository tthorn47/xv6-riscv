#include "kernel/types.h"
#include "user.h"

int main(){
    const char* name = "ThisIsABufferName";
    void* p;
    //printf("%p\n", p);
    ringbuf(name,0, &p);
    if(p==0)
        printf("Buffer not allocated.\n");
    else {
        printf("Buffer allocated. Somethings wrong.\n");
        ringbuf(name,1,&p);
        exit(1);
    }
    printf("%s\n",p);
    ringbuf(name,1,&p);

    exit(0);
}