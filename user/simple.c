#include "kernel/types.h"
#include "user.h"

int main(){
    const char* name = "ThisIsAName";
    ringbuf(name);
    exit(0);
}