#include "mylib.h"

int main(void) {
    static const char msg[] = "hello world\n";
    for (int i = 0; i < 3; i++) {
        mywrite(1, msg, sizeof(msg) - 1);
        mysleep(1);
    }
    myexit(0);
    return 0;
}
