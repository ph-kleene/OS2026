#include "common.h"
#include "device.h"

void kprintf(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    for (const char *p = fmt; *p; p++) {
        if (*p != '%') {
            putChar(*p);
            continue;
        }
        p++;
        if (*p == 'd') {
            putNum(va_arg(ap, int));
        } else if (*p == 'x') {
            uint32_t val = va_arg(ap, uint32_t);
            if (val == 0) putChar('0');
            else {
                char buf[9];
                int i = 8;
                buf[i--] = '\0';
                for (; val > 0; val /= 16) {
                    uint32_t r = val % 16;
                    buf[i--] = (r < 10) ? (r + '0') : (r - 10 + 'a');
                }
                putStr(buf + i + 1);
            }
        } else if (*p == 's') {
            char *s = va_arg(ap, char *);
            putStr(s ? s : "(null)");
        } else if (*p == '%') {
            putChar('%');
        } else {
            putChar('%');
            if (*p) putChar(*p);
        }
    }
    va_end(ap);
}

