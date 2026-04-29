CFLAGS  ?= -Wall -Wextra -g -O2
LDFLAGS ?=

.PHONY: all clean

all: multi_process_pi multi_thread_pi

multi_process_pi: multi_process_pi.c
	$(CC) $(CFLAGS) -o $@ $< -lm

multi_thread_pi: multi_thread_pi.c user_thread.c user_thread.h
	$(CC) $(CFLAGS) -o $@ user_thread.c multi_thread_pi.c -lm

clean:
	rm -f multi_process_pi multi_thread_pi
