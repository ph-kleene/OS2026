#ifndef __MYOS_H__
#define __MYOS_H__


void printf(const char *format,...);
void now(int *hh, int *mm, int *ss);
void sleep(int second);
int fork();
int vfork();
void exec(int sec, int num);
void exit();
int getpid();
int getppid();
int wait();
int sem_init(int value);
int sem_destroy(int sem_id);
int sem_post(int sem_id);
int sem_wait(int sem_id);

#endif
