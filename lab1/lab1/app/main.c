#include "main.h"

// Simple delay function
void delay() {
	volatile int i;
	for (i = 0; i < 100000; i++);
}

// Convert BCD to string
int bcdToStr(char bcd, char *buf) {
	// TODO: 把bcd转换为string并存入buf中
	buf[0] = ((bcd >> 4) & 0x0f) + '0';
	buf[1] = (bcd & 0x0f) + '0';
	
	return 2;
}

// Main program: display RTC time
int main(void) {
	char rtc_time_str[8] = "00:00:00";
	int row = 12;
	int col = (80 - 8) / 2;
	char hint_str[] = "Current Time:";
	displayStr(row - 1, (80 - 12) / 2, hint_str, 12);
	
	while (1) {
		// TODO: 实现展示时间的功能
		char hour = readRTC(0x04);
		char minute = readRTC(0x02);
		char second = readRTC(0x00);

		bcdToStr(hour, &rtc_time_str[0]);
		rtc_time_str[2] = ':';
		bcdToStr(minute, &rtc_time_str[3]);
		rtc_time_str[5] = ':';
		bcdToStr(second, &rtc_time_str[6]);

		displayStr(row, col, rtc_time_str, 8);
		
		delay();
	}
	return 0;
}
