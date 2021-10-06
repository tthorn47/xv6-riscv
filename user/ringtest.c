#include "kernel/types.h"
#include "ringbuffer.h"
#include "xorshift.c"
#include "user.h"
#define TEN_MEG (1024 * 1024 * 10)
#define TM_INT (TEN_MEG/4)

/* For 10MB transfer
 * Ringbuf  = 12 ticks or .83MB/S
 * Fastpipe = 26 ticks or .38MB/s
 * Basepipe = 65 ticks or .15MB/s
 * 
 */

int main() {
    char* name = "ThisIsAName";

    uint32 seed = getpid() + uptime() * 96;

    struct xorshift32_state* state = malloc(sizeof(struct xorshift32_state));
    state->a = seed;

    if(fork() == 0){
        
        sleep(10);
        int i = ringbuf_attach(name);
        int bytes;
        char* write;
        
        for(int j = 0; j < TEN_MEG; j+=bytes){
            bytes = 0;
            while(bytes < 4){
                ringbuf_start_write(i, &write, &bytes);
                //printf("writewait %d\n", j);
            }

            for(int mem = 0; mem < bytes/4; mem++)
                ((uint32*)write)[mem] = xorshift32(state);
            

            ringbuf_finish_write(i,bytes);
            
        } 
        ringbuf_release(i);
        exit(0);
    }

    int start = uptime();
    int bytes;
    char* read;
    int i = ringbuf_attach(name);

    for(int j = 0; j < TEN_MEG; j+=bytes){
        bytes = 0;
        while(bytes < 4){
            ringbuf_start_read(i, &read, &bytes);
            //printf("readwait %d\n", j);
        }

        for(int mem = 0; mem < bytes/4; mem++){
            uint32 check = xorshift32(state);
            if(check != ((uint32*)read)[mem]){
                 printf("Data Corruption!\n Val = %d | xor = %d | mem = %d \
                \nExiting!\n", ((uint32*)read)[mem], check, mem);
                exit(-1);
            }
                
        }
        ringbuf_finish_read(i,bytes);
        
    } 

    ringbuf_release(i);
    int fin = uptime() - start;
    printf("Total ticks = %d\n", fin);

    int status;
    wait(&status);

    exit(0);

    // int a = ringbuf_attach("a");
    // int b = ringbuf_attach("b");
    // int c = ringbuf_attach("c");
    // int d = ringbuf_attach("d");
    // int e = ringbuf_attach("e");
    // int f = ringbuf_attach("f");
    // int g = ringbuf_attach("g");
    // int h = ringbuf_attach("h");
    // i = ringbuf_attach("i");
    // int j = ringbuf_attach("j");
    // int k = ringbuf_attach("k");

        
    // exit(a+b+c+d+e+f+g+h+i+j+k);
}