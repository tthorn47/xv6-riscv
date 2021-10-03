#include "user.h"

#define MAX_BUFS 10
struct user_ring_buf {
    void *buf;
    void *book;
    int exists;
};
struct user_ring_buf* user_ringbufs[MAX_BUFS];

int ringbuf_attach(char* name) {
    void* p;

    for (int i = 0; i < MAX_BUFS; ++i) {
        if(user_ringbufs[i] == 0) {
            ringbuf(name,0, &p);
            user_ringbufs[i]->buf = p;
            user_ringbufs[i]->exists = 1;
            printf("%p\n", user_ringbufs[i]->buf);
            return i;
        }
    }

    return 1;
}

void ringbuf_release(char* name, int buffer_id) {
    ringbuf(name, 1, user_ringbufs[buffer_id]->buf);
    return;
}

void ringbuf_start_read(int ring_desc, char **addr, int *bytes) {

}

void ringbuf_finish_read(int ring_desc, int bytes) {

}

void ringbuf_start_write(int ring_desc, char **addr, int *bytes) {

}

void ringbuf_finish_write(int ring_desc, int bytes){

}