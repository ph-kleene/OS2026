#include "ringBuffer.h"

#include <stdexcept>

ringBuffer::ringBuffer(size_t capacity)
    : cap_(capacity), head_(0), tail_(0), count_(0)
{
    if (capacity == 0)
        throw std::invalid_argument("ringBuffer: capacity must be > 0");

    buf_ = new int[capacity];

    pthread_mutex_init(&mutex_, nullptr);
    pthread_cond_init(&not_full_, nullptr);
    pthread_cond_init(&not_empty_, nullptr);
}

ringBuffer::~ringBuffer()
{
    delete[] buf_;
    pthread_mutex_destroy(&mutex_);
    pthread_cond_destroy(&not_full_);
    pthread_cond_destroy(&not_empty_);
}

void ringBuffer::put(int value)
{
    pthread_mutex_lock(&mutex_);

    /* wait while the buffer is full */
    while (count_ == cap_)
        pthread_cond_wait(&not_full_, &mutex_);

    buf_[head_] = value;
    head_ = (head_ + 1) % cap_;
    count_++;

    /* signal one waiting consumer */
    pthread_cond_signal(&not_empty_);
    pthread_mutex_unlock(&mutex_);
}

void ringBuffer::get(int &out)
{
    pthread_mutex_lock(&mutex_);

    /* wait while the buffer is empty */
    while (count_ == 0)
        pthread_cond_wait(&not_empty_, &mutex_);

    out = buf_[tail_];
    tail_ = (tail_ + 1) % cap_;
    count_--;

    /* signal one waiting producer */
    pthread_cond_signal(&not_full_);
    pthread_mutex_unlock(&mutex_);
}

size_t ringBuffer::size() const
{
    pthread_mutex_lock(const_cast<pthread_mutex_t *>(&mutex_));
    size_t n = count_;
    pthread_mutex_unlock(const_cast<pthread_mutex_t *>(&mutex_));
    return n;
}

size_t ringBuffer::capacity() const
{
    return cap_;
}

bool ringBuffer::empty() const
{
    pthread_mutex_lock(const_cast<pthread_mutex_t *>(&mutex_));
    bool e = (count_ == 0);
    pthread_mutex_unlock(const_cast<pthread_mutex_t *>(&mutex_));
    return e;
}

bool ringBuffer::full() const
{
    pthread_mutex_lock(const_cast<pthread_mutex_t *>(&mutex_));
    bool f = (count_ == cap_);
    pthread_mutex_unlock(const_cast<pthread_mutex_t *>(&mutex_));
    return f;
}
