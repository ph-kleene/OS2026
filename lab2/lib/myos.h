#ifndef __MYOS_H__
#define __MYOS_H__


void printf(const char *format,...);
void now(int *hh, int *mm, int *ss);
void zeroTimeTicks();
int getTimeTicks();
void sleep(int second);

#endif
