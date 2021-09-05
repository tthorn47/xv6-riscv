#include "xorshift.c"
#include "user.h"

#define TEN_MEG 10000000
#define TM_INT 2500000

int main(){
    int k, i;
    int flags[2];  
    uint32 seed = getpid() + uptime() * 96;
    uint32* writeVals = malloc(TEN_MEG);

    struct xorshift32_state* state = malloc(sizeof(struct xorshift32_state));
    state->a = seed;

    for (i = 0; i < TM_INT; i++)
    {
        writeVals[i] = xorshift32(state);
    }
    printf("Send = %d\n", writeVals[0]);
    if((k = pipe(flags)) < 0){
        exit(k);
    }

    

    if(fork() == 0){
        int err;
        //Since write loops internally, no need to complicate outside code
        if((err = write(flags[1], writeVals, TEN_MEG) < 0)){
            printf("couldn't write %d", err);
            exit(0);
        }          

        exit(0);
    }


    int start = uptime();
    uint32* readVals = malloc(TEN_MEG);
    int err;
    int readCount = 0;
    
    while(readCount < TEN_MEG){
        //Loop on 512 is the best fit, it never reads more than that anyway
        if((err = read(flags[0], ((char*)readVals)+readCount, 1024)) < 0){
            printf("couldn't read %d", err);
            exit(0);
        }
        if(readCount == 0){
            printf("First Read %d\n", readVals[0]);
        }
        readCount+=(err);
    }
    
    int fin = uptime() - start;
    printf("Total ticks = %d\n", fin);

    int checkCount = 0;
    printf("before %d\n", readVals[checkCount]);
    printf("rand %d | %d", readVals[16900], writeVals[16900]);
    //check against another xorstate just to be sure
    struct xorshift32_state* checkState = malloc(sizeof(struct xorshift32_state));
    checkState->a = seed;
    while(checkCount < TM_INT){
        
        int check = xorshift32(checkState);
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