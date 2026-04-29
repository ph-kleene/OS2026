#ifndef MYLIB_H
#define MYLIB_H

#ifdef __cplusplus
extern "C" {
#endif

long mywrite(int fd, const void *buf, unsigned int count);
int mysleep(unsigned int sec);
void myexit(int status);

#ifdef __cplusplus
}
#endif

#endif
