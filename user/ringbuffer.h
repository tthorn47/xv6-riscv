#include "user.h"

#define BUF_PAGES 16
#define PAGE 4096
#define BUF_SIZE BUF_PAGES * PAGE
#define MAX_BUFS 20

struct user_ring_buf {
    void *buf;
    void *read_ptr;
    void *write_ptr;
    void *book;
    int exists;
};

struct user_ring_buf* user_ringbufs[MAX_BUFS];

// Used to attach to a ring buffer with *name*.
// Returns the index of ring buffer in array. -1 if buffer cannot be created if it does not already exist.
int ringbuf_attach(char* name) {
    void* p;
    int status = ringbuf(name,0, &p);

    if(status == -1) {
        printf("Error: ringbuf system call errored out\n");
        return -1;
    } else {
        for (int i = 0; i < MAX_BUFS; ++i) {
            if (user_ringbufs[i] == 0) {
                user_ringbufs[i]->buf = p;
                user_ringbufs[i]->read_ptr=p;
                user_ringbufs[i]->write_ptr=p;
                user_ringbufs[i]->exists = 1;
                printf("%p\n", user_ringbufs[i]->buf);
                return i;
            }
        }
    }

    printf("Max number of file descriptors reached")
    return -1;
}

// Used to detach from a ring buffer with name and position buffer_id in array of descriptors.
void ringbuf_release(char* name, int buffer_id) {
    int status = ringbuf(name, 1, user_ringbufs[buffer_id]->buf);
    ringbufs[buffer_id] = 0;

    if(status == -1) {
        printf("Error: ringbuf system call errored out\n");
    }

    return;
}

// 1) Assign start position for reading to addr.
// 2) Calculate number of bytes available for reading and assign to bytes.
//     write_ptr-read_ptr (Assuming write pointer can never be less than read pointer as
//     we will always pull both pointers back to the beginning if either goes beyond the buffer_size from start.
//     Also the difference cannot be greater than buffer_size)
void ringbuf_start_read(int ring_desc, char **addr, int *bytes) {

}

void ringbuf_finish_read(int ring_desc, int bytes) {

}

// 1) Assign start position for writing to addr.
// 2) Calculate number of bytes available for writing and assign to bytes.
//     BUF_SIZE-(write_ptr-read_ptr)
void ringbuf_start_write(int ring_desc, char **addr, int *bytes) {

}

void ringbuf_finish_write(int ring_desc, int bytes){

}