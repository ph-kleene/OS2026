CC      = gcc
CXX     = g++
CFLAGS  = -Wall -Wextra -O2 -pthread
CXXFLAGS = -Wall -Wextra -O2 -pthread -std=c++17

.PHONY: all clean

all: q1_rwlock_test q2_ringbuffer_test

q1_rwlock_test: q1_src/rwlock_test.c
	$(CC) $(CFLAGS) -o $@ $<

q2_ringbuffer_test: q2_src/ringBuffer.cpp q2_src/test_ringbuffer.cpp
	$(CXX) $(CXXFLAGS) -o $@ $^

clean:
	rm -f q1_rwlock_test q2_ringbuffer_test
