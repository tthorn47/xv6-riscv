#include "kernel/types.h"
#include "user.h"

int main(){
    int pipes[2];
    pipe(pipes);
    char k = 69;
    char* j = malloc(1);
    write(pipes[1], &k, 4);
    read(pipes[0], j, 4);

    printf("%d\n", *j);
    free(j);
    exit(0);
}