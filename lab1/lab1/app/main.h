#ifndef MAIN_H
#define MAIN_H

// Display string at position
void displayStr(int row, int col, char *str, int len);
// Clear string at position
void clearStr(int row, int col, int len);
// Read from RTC register，readRTC(0x00)读取秒，readRTC(0x02)读取分钟，readRTC(0x04)读取小时
char readRTC(int reg);

#endif
