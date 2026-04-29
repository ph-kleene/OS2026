#include "myos.h"

#define APP_TEST_FULL 1
#define APP_TEST_FORK_SHARE 2
#define APP_TEST_VFORK_SHARE 3

#define APP_TEST_MODE APP_TEST_FULL

#ifndef APP_TEST_MODE
#define APP_TEST_MODE APP_TEST_FULL
#endif

/* TODO 1: 添加全局变量用于测试fork/vfork的内存共享特性
 * - 定义一个全局变量（如计数器）
 * - 在fork/vfork后，父子进程分别修改该变量
 * - 通过输出观察值的变化，验证fork为独立内存、vfork为共享内存
 */
/* TODO: 定义全局变量 */
volatile int g_shared = 100;

int main(void) {
	printf("User App Started. My PID: %d, PPID: %d\r\n", getpid(), getppid());

#if APP_TEST_MODE == APP_TEST_FORK_SHARE
	printf("[Fork Share Test] Initial g_shared = %d\r\n", g_shared);
	int f = fork();
	if (f == 0) {
		g_shared = 200;
		printf("[Fork Share Test] Child PID %d changed g_shared to %d\r\n", getpid(), g_shared);
		sleep(1);
		exit();
	}
	sleep(2);
	printf("[Fork Share Test] Parent PID %d sees g_shared = %d (expected 100)\r\n", getpid(), g_shared);
	while (1) {
		sleep(5);
	}
#endif

#if APP_TEST_MODE == APP_TEST_VFORK_SHARE
	printf("[vFork Share Test] Initial g_shared = %d\r\n", g_shared);
	int v = vfork();
	if (v == 0) {
		g_shared = 300;
		printf("[vFork Share Test] Child PID %d changed g_shared to %d\r\n", getpid(), g_shared);
		exit();
	}
	sleep(2);
	printf("[vFork Share Test] Parent PID %d sees g_shared = %d (expected 300)\r\n", getpid(), g_shared);
	while (1) {
		sleep(5);
	}
#endif
	
	/* APP_TEST_FULL */
	// Test Fork Failure
	printf("Testing Fork Failure (exhausting slots)...\r\n");
	for (int i = 0; i < 6; i++) {
		int pid = fork();
		if (pid == 0) {
			// Child process
			int my_pid = getpid();
			printf("Child PID %d created. Sleeping...\r\n", my_pid);
			sleep(10); // Stay alive long enough to exhaust slots
			printf("Child PID %d exiting.\r\n", my_pid);
			exit();
		} else if (pid == -1) {
			printf("Fork failed as expected! (No more slots available)\r\n");
		} else {
			printf("Parent created child with PID %d\r\n", pid);
		}
	}

	// Test vfork (available if slots freed up, which they should be after 10s sleeps in children above expire)
	// We wait a bit to ensure some children exit
	sleep(12);

	printf("\r\nTesting vfork concurrency control...\r\n");
	int v1 = vfork();
	if (v1 == 0) {
		printf("vfork Child 1 (PID %d) created, sleeping before exec...\r\n", getpid());
		sleep(3);
		exec(129, 128);
	} else if (v1 > 0) {
		printf("Parent: Created vfork child 1 (PID %d), attempting vfork 2...\r\n", v1);
		int v2 = vfork();
		if (v2 == -1) {
			printf("Parent: vfork 2 failed as expected (Concurrency Control Works!)\r\n");
		} else if (v2 == 0) {
			printf("vfork Child 2 (PID %d) - ERROR: This should not happen!\r\n", getpid());
			exit();
		} else {
			printf("Parent: vfork 2 succeeded with PID %d - ERROR: Concurrency control failed!\r\n", v2);
		}
	}

	printf("Parent PID %d going into main loop.\r\n", getpid());

	while(1) {
		int h, m, s;
		now(&h, &m, &s);
		printf("Parent (PID %d) Time: %2d:%2d:%2d\r\n", getpid(), h, m, s);
		sleep(5);
	}
}
