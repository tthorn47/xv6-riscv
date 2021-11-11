#include "kernel/types.h"
#include "user.h"

void main(){
    int f = fork();


    if(f == 0){
        uint64 buf;
        printf("Barrier 0: %d\n", read(4, &buf, 4));
        printf("Barrier 1: %d\n", read(5, &buf, 4));
        printf("Barrier 2: %d\n", read(6, &buf, 4));
        printf("Barrier 3: %d\n", read(7, &buf, 4));
        printf("Barrier 4: %d\n", read(8, &buf, 4));
        printf("Barrier 5: %d\n", read(9, &buf, 4));
        printf("Barrier 6: %d\n", read(10, &buf, 4));
        printf("Barrier 7: %d\n", read(11, &buf, 4));
        printf("Barrier 8: %d\n", read(12, &buf, 4));
        printf("Barrier 9: %d\n", read(13, &buf, 4));
        int k = fork();
        if(k == 0){
            printf("Four blurry outputs should follow:\n");
            printf("Barrier 0: %d\n", read(4, &buf, 4));
            printf("Barrier 1: %d\n", read(5, &buf, 4));
            printf("Barrier 2: %d\n", read(6, &buf, 4));
            printf("Barrier 3: %d\n", read(7, &buf, 4));
        } else {
            printf("Barrier 0: %d\n", read(4, &buf, 4));
            printf("Barrier 1: %d\n", read(5, &buf, 4));
            printf("Barrier 2: %d\n", read(6, &buf, 4));
            printf("Barrier 3: %d\n", read(7, &buf, 4));
        }
        exit(0);
    }
    uint64 buf = 0;
    sleep(10);
    write(4, &buf, 4);
    sleep(10);
    write(5, &buf, 4);
    sleep(10);
    write(6, &buf, 4);
    sleep(10);
    write(7, &buf, 4);
    sleep(10);
    write(8, &buf, 4);
    sleep(10);
    write(9, &buf, 4);
    sleep(10);
    write(10, &buf, 4);
    sleep(10);
    write(11, &buf, 4);
    sleep(10);
    write(12, &buf, 4);
    sleep(10);
    write(13, &buf, 4);
    sleep(25);
    write(4, &buf, 4);
    sleep(10);
    write(5, &buf, 4);
    sleep(10);
    write(6, &buf, 4);
    sleep(10);
    write(7, &buf, 4);
    sleep(10);
    exit(0);
}