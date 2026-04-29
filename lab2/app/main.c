#include "myos.h"

#define TEST_CASE_NONE 0
#define TEST_CASE_OOB 1
#define TEST_CASE_PRIV 2

// 切换测试模式：
// TEST_CASE_NONE: 仅展示RTC
// TEST_CASE_OOB:  地址越界异常（配合 make bochs）
// TEST_CASE_PRIV: 特权指令异常（配合 make run）
#define TEST_CASE TEST_CASE_NONE

int main(void) {

	
	int hh, mm, ss;
	printf("<Real Time Clock>\n");

#if TEST_CASE == TEST_CASE_NONE
	while (1) {
		now(&hh, &mm, &ss);
		printf("\rCurrent Time: %2d:%2d:%2d", hh, mm, ss);
		sleep(1);
	}
#else
	for(int i=0; i<10; i++) {
		now(&hh, &mm, &ss);
		printf("\rCurrent Time: %2d:%2d:%2d", hh, mm, ss);
		sleep(1);
	}
#endif

#if TEST_CASE == TEST_CASE_OOB
	// TODO1：地址访问越界
	*(volatile int *)0x30000000 = 1;
#elif TEST_CASE == TEST_CASE_PRIV
	// TODO2：特权指令
	asm volatile("hlt");
#endif

        //注意：两个todo可以都一起完成，跟在现有代码后面，先用make bochs测试TODO1，再用make run测试TODO2（make run用的是qemu，TODO1不影响测试结果）
	return 0;
}
