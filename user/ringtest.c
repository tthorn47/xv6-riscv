#include "kernel/types.h"
#include "ringbuffer.h"
#include "xorshift.c"
#include "user.h"
#define TEN_MEG (1024 * 1024 * 10)
#define TM_INT (TEN_MEG/4)


int main() {
    char* name = "ThisIsAName";

    // int i = ringbuf_attach(name);
    // char *point;
    // char *rPoint;
    // int  store;
    // printf("before start %p\n", point);
    // ringbuf_start_write(i, &point, &store);
    // printf("after start %p\n", point);
    // *(int*)point = 69;
    // ringbuf_finish_write(i, 4);
    // ringbuf_start_write(i, &point, &store);
    // printf("after finish %p\n", point);

    // printf("before r start %p\n", rPoint);
    // ringbuf_start_read(i, &rPoint, &store);
    // printf("after r start %p\n", rPoint);
    // printf("%d\n", *(int*)rPoint);
    // ringbuf_finish_read(i, 4);
    // ringbuf_start_read(i, &rPoint, &store);
    // printf("after r finish %p\n", rPoint);
    // ringbuf_release(i);


    int k;
    uint32 seed = getpid() + uptime() * 96;
    uint32* writeVals = malloc(TEN_MEG);

    struct xorshift32_state* state = malloc(sizeof(struct xorshift32_state));
    state->a = seed;

    for (k = 0; k < TM_INT; k++)
    {
        writeVals[k] = xorshift32(state);
    }
    printf("Send = %d %d \n", writeVals[0], sizeof(writeVals[0]));

    

    if(fork() == 0){
        
        sleep(10);
        int i = ringbuf_attach(name);
        int bytes = -1;
        char* write;
        
        for(int j = 0; j < TEN_MEG/4; j+=8){
            while(bytes < 32){
                ringbuf_start_write(i, &write, &bytes);
                //printf("writewait %d\n", j);
            }

            // if(j < 4){
            //     printf("book %p %d %p\n", books[0]->top , (books[0]->nWrite % BUF_SIZE), books[0]->top+(books[0]->nWrite % BUF_SIZE));
            // }
            
            ((uint32*)write)[0] = writeVals[j];
            ((uint32*)write)[1] = writeVals[j+1];
            ((uint32*)write)[2] = writeVals[j+2];
            ((uint32*)write)[3] = writeVals[j+3];
            ((uint32*)write)[4] = writeVals[j+4];
            ((uint32*)write)[5] = writeVals[j+5];
            ((uint32*)write)[6] = writeVals[j+6];
            ((uint32*)write)[7] = writeVals[j+7];

            ringbuf_finish_write(i,32);
            bytes = -1;
        } 
        //printf("child release\n");
        ringbuf_release(i);
        exit(0);
    }


    int start = uptime();
    uint32* readVals = malloc(TEN_MEG);
    
    int bytes = -1;
    char* read;
    int i = ringbuf_attach(name);
    for(int j = 0; j < TEN_MEG/4; j+=8){
        while(bytes < 32){
            ringbuf_start_read(i, &read, &bytes);
            //printf("readwait %d\n", j);
        }
        
        readVals[j]   = ((uint32*)read)[0];
        readVals[j+1] = ((uint32*)read)[1];
        readVals[j+2] = ((uint32*)read)[2];          
        readVals[j+3] = ((uint32*)read)[3];
        readVals[j+4] = ((uint32*)read)[4];
        readVals[j+5] = ((uint32*)read)[5];
        readVals[j+6] = ((uint32*)read)[6];
        readVals[j+7] = ((uint32*)read)[7];
        ringbuf_finish_read(i,32);
        bytes = -1;
    } 
    //printf("parent release\n");
    ringbuf_release(i);
    int fin = uptime() - start;
    printf("Total ticks = %d\n", fin);

    int checkCount = 0;
    //check against another xorstate just to be sure
    struct xorshift32_state* checkState = malloc(sizeof(struct xorshift32_state));
    checkState->a = seed;
    while(checkCount < TM_INT){
        
        uint32 check = xorshift32(checkState);
        if(readVals[checkCount] != check){
            printf("Data Corruption!\n Val = %d | xor = %d | checkCount = %d \
                \nExiting!\n", readVals[checkCount], check, checkCount);
            exit(-1);
        }
        checkCount++;
    }
    int status;
    wait(&status);
    exit(0);
}