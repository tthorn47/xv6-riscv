#include "kernel/types.h"
#include "user.h"

int main(){
    const char* name = "ThisIsAName";
    void* pointer = malloc(sizeof(void*));

    ringbuf(name,0,&pointer);
    ringbuf(name,1,&pointer);
    exit(0);
}