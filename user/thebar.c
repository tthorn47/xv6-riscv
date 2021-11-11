#include "kernel/types.h"
#include "user.h"

int main(){
    int i = fork();

    if(i == 0){
        int j = fork();
        int k = fork();
        int l = fork();
        int m = fork();
        int d;
        read(4, &d, 4);
        sleep(i + j + k + l + m);
        printf("check : %d\n", d);
        exit(0);
    }
    int d;
    sleep(50);
    write(4, &d, 4);
    sleep(125);
    exit(0);
}