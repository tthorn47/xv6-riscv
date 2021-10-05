#include "user.h"
#include "kernel/types.h"
#define BUF_PAGES 16
#define PAGE 4096
#define BUF_SIZE (BUF_PAGES * PAGE)
#define MAX_BUFS 10


struct book {
    void* top;      //start of the ring buffer
    void* write;    //current write head
    void* read;     //current read head
    uint64 nRead;
    uint64 nWrite;
    char name[16];  //name of the buffer
};

//const int SIZE = BUF_SIZE;
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
            if(i != -1){
                return i;
            }
            i = get_slot();
            if(strcmp(the_book->name, name) != 0){                            
                __atomic_store_8(&the_book->top, (uint64)addr, __ATOMIC_SEQ_CST);
                __atomic_store_8(&the_book->write, (uint64)addr, __ATOMIC_SEQ_CST);
                __atomic_store_8(&the_book->read, (uint64)addr, __ATOMIC_SEQ_CST);
                __atomic_store_8(&the_book->nRead, (uint64)0, __ATOMIC_SEQ_CST);
                __atomic_store_8(&the_book->nWrite, (uint64)0, __ATOMIC_SEQ_CST);
                strcpy(&the_book->name[0], name);                              
            }else{
                printf("found\n");
            }
            __atomic_store_8(&books[i], (uint64)the_book, __ATOMIC_SEQ_CST);
            return i;
    }
}

// Used to detach from a ring buffer with name and position buffer_id in array of descriptors.
void ringbuf_release(int id) {
    struct book* the_book = find_book(id);
    
    if(the_book == 0){
        printf("release: Buffer not found");
        return;
    }

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
    if (book->name == 0)
        return -1;

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

 // 1) Assign start position for reading to addr.
 // 2) Calculate number of bytes available for reading and assign to bytes.
 //     (write_ptr-read_ptr) % BUF_SIZE

void ringbuf_start_read(int ring_desc, char **addr, int *bytes) {
    struct book* reader = find_book(ring_desc);
//     printf("%p\n", reader->read);
//     printf("%s\n", reader->read);
//     printf("%d\n", (int)BUF_SIZE);

    *addr = reader->read;
    *bytes = (reader->write-reader->read) % BUF_SIZE;

    return;
}

// 1) Move read head forward by *bytes*.
void ringbuf_finish_read(int ring_desc, int bytes) {
    struct book* reader = find_book(ring_desc);
    reader->read += bytes;

//    printf("%p\n", reader->read);
//    printf("%p\n", reader->write);
}

// // 1) Assign start position for writing to addr.
// // 2) Calculate number of bytes available for writing and assign to bytes.
// //     BUF_SIZE-(write_ptr-read_ptr)
// void ringbuf_start_write(int ring_desc, char **addr, int *bytes) {

void ringbuf_finish_read(int ring_desc, int bytes) {
    
    struct book* the_book = find_book(ring_desc);
    __atomic_store_8(&the_book->nRead, (uint64)(the_book->nRead + bytes), __ATOMIC_SEQ_CST);
    __atomic_store_8(&the_book->read, (uint64)(the_book->top + (the_book->nRead % BUF_SIZE)), __ATOMIC_SEQ_CST);
}

// 1) Assign start position for writing to addr.
// 2) Calculate number of bytes available for writing and assign to bytes.
//     BUF_SIZE-(write_ptr-read_ptr)
void ringbuf_start_write(int ring_desc, char **addr, int *bytes) {
    struct book* the_book = find_book(ring_desc);

    //set the *addr to the start of write book->write
    *addr = (char*)__atomic_load_8(&the_book->write, __ATOMIC_SEQ_CST);
    //do the calculation for the available space
    if (__atomic_load_8(&the_book->nWrite, __ATOMIC_SEQ_CST) == BUF_SIZE + __atomic_load_8(&the_book->nRead, __ATOMIC_SEQ_CST))
        *bytes = 0;
    else
        *bytes = BUF_SIZE - (__atomic_load_8(&the_book->nWrite, __ATOMIC_SEQ_CST) - __atomic_load_8(&the_book->nRead, __ATOMIC_SEQ_CST));
}

void ringbuf_finish_write(int ring_desc, int bytes){
    struct book* the_book = find_book(ring_desc);
    __atomic_store_8(&the_book->nWrite, (uint64)(the_book->nWrite + bytes), __ATOMIC_SEQ_CST);
    __atomic_store_8(&the_book->write, ((uint64)the_book->top + (the_book->nWrite % BUF_SIZE)), __ATOMIC_SEQ_CST);
}