#include "kernel/types.h"
#include "ringbuffer.h"

int main() {
    char* name = "ThisIsAName";

    char* string_to_write = "SomeRandomNames";
    char* copy_of_stw = string_to_write;
    int string_length = 15;
    int length_to_read = string_length;

    int i = ringbuf_attach(name);

    // printf("%p\n", books[i]->write);

    // Writing using write head and moving it forward.
    while(length_to_read-- > 0 && (*(char*)books[i]->write++ = *string_to_write++) != 0);
//    *(char*)(books[i]->write+string_length+1) = '\0';


    // printf("%p\n", books[i]->read);


    // Testing read
    char* read_buffer;
    int bytes;
    ringbuf_start_read(i, &read_buffer, &bytes);

//     printf("%p\n", read_buffer);
//     printf("%d\n", bytes);

    while(string_length > 0 && *(char*)books[i]->read && *(char*)books[i]->read == *copy_of_stw)
        string_length--, books[i]->read++, copy_of_stw++;
    if(string_length==0)
        printf("Test Passed\n");
    else
        printf("Test Failed\n");

    ringbuf_finish_read(i, bytes);


    ringbuf_release(i);

    exit(0);
}