#include "user.h"

struct user_ring_buf {
    void *buf;
    void *book;
    int exists;
};

void ringbuf_start_read(int ring_desc, char **addr, int *bytes);
void ringbuf_finish_read(int ring_desc, int bytes);

void ringbuf_start_write(int ring_desc, char **addr, int *bytes);
void ringbuf_finish_write(int ring_desc, int bytes);