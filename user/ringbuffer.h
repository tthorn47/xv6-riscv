#include "user.h"
#include "kernel/types.h"
#define BUF_PAGES 16
#define PAGE 4096
#define BUF_SIZE BUF_PAGES * PAGE
#define MAX_BUFS 10

struct book {
    void* top;      //start of the ring buffer
    void* write;    //current read head
    void* read;     //current write head
    char name[16]; //name of the buffer
};

struct book* books[MAX_BUFS];
int ringbuf_attach(char* name);
void ringbuf_release(int buffer_id);
int get_slot();
int get_index(struct book*);
struct book* find_book(int);
// Used to attach to a ring buffer with *name*.
// Returns the index of ring buffer in array. -1 if buffer cannot be created if it does not already exist.
int ringbuf_attach(char* name) {
    void* addr;
    if((ringbuf(name, 0, &addr)) < 0){
        return -1;
    }else{
            
            struct book* the_book = (struct book*)(addr+(BUF_SIZE*2));
            int i = get_index(the_book);
            if(i == -1){  
                i = get_slot();             
                __atomic_store_8(&the_book->top, (uint64)addr, __ATOMIC_SEQ_CST);
                __atomic_store_8(&the_book->write, (uint64)addr, __ATOMIC_SEQ_CST);
                __atomic_store_8(&the_book->read, (uint64)addr, __ATOMIC_SEQ_CST);
                strcpy(&the_book->name[0], name);                     
                __atomic_store_8(&books[i], (uint64)the_book, __ATOMIC_SEQ_CST);
            }
            return i;
    }
}

// Used to detach from a ring buffer with name and position buffer_id in array of descriptors.
void ringbuf_release(int id) {
    struct book* the_book = find_book(id);
    
    if(the_book == 0)
        return;

    ringbuf(the_book->name, 1, &the_book->top);   
}

int get_slot(){
    for (int i = 0; i < 10; i++)
    {
        if(books[i] == 0)
            return i;
    }
    return -1;
}

int get_index(struct book* book){
    for (int i = 0; i < 10; i++)
    {
        if(strcmp(books[i]->name, book->name) == 0)
            return i;
    }

    return -1;
}

struct book* find_book(int id){
    if (books[id] != 0)
    {
        return books[id];
    }
    
    return 0;
}

// // 1) Assign start position for reading to addr.
// // 2) Calculate number of bytes available for reading and assign to bytes.
// //     write_ptr-read_ptr (Assuming write pointer can never be less than read pointer as
// //     we will always pull both pointers back to the beginning if either goes beyond the buffer_size from start.
// //     Also the difference cannot be greater than buffer_size)
// void ringbuf_start_read(int ring_desc, char **addr, int *bytes) {

// }

// void ringbuf_finish_read(int ring_desc, int bytes) {

// }

// // 1) Assign start position for writing to addr.
// // 2) Calculate number of bytes available for writing and assign to bytes.
// //     BUF_SIZE-(write_ptr-read_ptr)
// void ringbuf_start_write(int ring_desc, char **addr, int *bytes) {

// }

// void ringbuf_finish_write(int ring_desc, int bytes){

// }