#ifndef RINGBUFFER_H
#define RINGBUFFER_H

#include <pthread.h>
#include <cstddef>

class ringBuffer {
public:
    explicit ringBuffer(size_t capacity);
    ~ringBuffer();

    /* copy / move prohibited — owns mutex/cv resources */
    ringBuffer(const ringBuffer &) = delete;
    ringBuffer &operator=(const ringBuffer &) = delete;

    /* blocking put: waits while buffer is full */
    void put(int value);

    /* blocking get: waits while buffer is empty, writes to *out */
    void get(int &out);

    /* non-blocking queries (best-effort; may be stale by the time you
       read the return value, but useful for logging / debugging) */
    size_t size() const;
    size_t capacity() const;
    bool   empty() const;
    bool   full() const;

private:
    int            *buf_;
    size_t          cap_;
    size_t          head_;   /* next write position */
    size_t          tail_;   /* next read position  */
    size_t          count_;  /* current item count   */

    pthread_mutex_t mutex_;
    pthread_cond_t  not_full_;
    pthread_cond_t  not_empty_;
};

#endif
