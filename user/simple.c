#include "kernel/types.h"
#include "user.h"
#include "ringbuffer.h"

#define BUF_PAGES 16
#define PAGE 4096
#define BUF_SIZE (BUF_PAGES * PAGE)
#define MAX_BUFS 10

int main(){
        char* point;
        int bytes = -1;
        int f = fork();

        if(f == 0){
            int i = ringbuf_attach("name");
            ringbuf_start_write(i, &point, &bytes);
            printf("child %p | %d\n", point, bytes);
            *point = 'a';
            ringbuf_finish_write(i, BUF_SIZE);
            ringbuf_start_write(i, &point, &bytes);
            printf("child %p | %d\n", point, bytes);
            printf("child %d | %d\n", books[0]->nRead, books[0]->nWrite);
            sleep(10);
            ringbuf_release(i);
            exit(0);
        }
        sleep(3);
        int i = ringbuf_attach("name");
        printf("ping\n");
        while(bytes == -1)
            ringbuf_start_read(i, &point, &bytes);
        
        ringbuf_finish_read(i, BUF_SIZE);

        ringbuf_start_write(i, &point, &bytes);
        printf("parent %c %p | %d\n", *point, point, bytes);
        ringbuf_start_read(i, &point, &bytes);
        printf("parent %c %p | %d\n", *point, point, bytes);
        printf("parent %d | %d\n", books[0]->nRead, books[0]->nWrite);

        ringbuf_release(i);
        
    exit(0);
}