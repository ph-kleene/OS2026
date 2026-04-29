#include "../kernel/include/common/stdarg.h"
#include "../kernel/include/syscall.h"

// System Call Numbers moved to syscall.h

#define MAX_BUFFER_SIZE 256

int syscall(int num, unsigned int a1, unsigned int a2, unsigned int a3, unsigned int a4, unsigned int a5);

void now(int *hh, int *mm, int *ss){
	syscall(SYS_NOW, (unsigned int)hh, (unsigned int)mm, (unsigned int)ss, 0, 0);
}

int fork() {
	return syscall(SYS_FORK, 0, 0, 0, 0, 0);
}

int vfork() {
	return syscall(SYS_VFORK, 0, 0, 0, 0, 0);
}

void exec(int sec, int num) {
	syscall(SYS_EXEC, (unsigned int)sec, (unsigned int)num, 0, 0, 0);
}

void exit() {
	syscall(SYS_EXIT, 0, 0, 0, 0, 0);
}

void sleep(int second) {
	syscall(SYS_SLEEP, (unsigned int)(second * 100), 0, 0, 0, 0);
}

int getpid() {
	return syscall(SYS_GETPID, 0, 0, 0, 0, 0);
}

int getppid() {
	return syscall(SYS_GETPPID, 0, 0, 0, 0, 0);
}

static int dec2Str(int n, char *buf, int count, int w) {
	char s[16];
	int i = 0, neg = n < 0;
	if (neg) n = -n;
	do { s[i++] = (n % 10) + '0'; n /= 10; } while (n > 0);
	if (neg) s[i++] = '-';
	while (i < w) s[i++] = '0';
	while (i > 0) {
		buf[count++] = s[--i];
		if (count == MAX_BUFFER_SIZE) {
			syscall(SYS_WRITE, (unsigned int)buf, (unsigned int)MAX_BUFFER_SIZE, 0, 0, 0);
			count = 0;
		}
	}
	return count;
}

void printf(const char *format,...){
	va_list args;
	va_start(args,format);
	char buffer[MAX_BUFFER_SIZE];
	int count=0;
	for (int i=0; format[i]; i++) {
		if (format[i] == '%') {
			int width = 0;
			i++;
			while (format[i] >= '0' && format[i] <= '9') width = width * 10 + (format[i++] - '0');
			if (format[i] == 'd') {
				count = dec2Str(va_arg(args, int), buffer, count, width);
			}
		} else {
			buffer[count++] = format[i];
		}
		if (count == MAX_BUFFER_SIZE) {
			syscall(SYS_WRITE, (unsigned int)buffer, (unsigned int)MAX_BUFFER_SIZE, 0, 0, 0);
			count = 0;
		}
	}
	if (count > 0) syscall(SYS_WRITE, (unsigned int)buffer, (unsigned int)count, 0, 0, 0);
	va_end(args);
}