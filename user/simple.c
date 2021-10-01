#include "kernel/types.h"
#include "user.h"

int main(){
    const char* name = "ThisIsAName";
    void* p;
    //printf("%p\n", p);
    ringbuf(name,0, &p);
    //printf("%p\n", p);

    p = "This was stored in the ring buffer!";
    printf("%s\n",p);
    

    const char* o_name = "Diffname";
    void* x;
    ringbuf(o_name,0,&x);
    x = "This was stored in a different ring buffer!";
    printf("%s\n",x);
    ringbuf(o_name,1,&x);
    ringbuf(name,1,&p);

    ringbuf(name,0,&p);

    p = "This was stored in a ring buffer with the same name as the first ring buffer!";
    printf("%s\n",p);
    ringbuf(name,1,&p);
    //free(p);
    //free(x);


    int f = fork();

    if(f == 0){
        void* parent;
        ringbuf("test", 0, &parent);
        sleep(15);
        printf("%c\n", ((char*)parent)[0]);
        ringbuf("test", 1, &parent);
        exit(0);
    }
        // void* child;
        // int stat = ringbuf("test", 0, &child);
        // printf("%d\n", stat);
        // child = "Exit test";
        // exit(0);
    // } else {
        //int stat;
        //printf("pid = %d\n", f);
        void* parent;
        ringbuf("test", 0, &parent);
        ((char*)parent)[0] = 'a';
        printf("%c\n", parent);
        ringbuf("test", 1, &parent);
        //wait(&stat);
    //}
    exit(0);
}