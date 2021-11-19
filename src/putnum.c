#include <stdio.h>

void putnum(long n) {
    char suffix;
    if (n >= 1000000) {
        n /= 100000;
        suffix = 'M';
    } else if (n >= 1000) {
        n /= 100;
        suffix = 'k';
    } else {
        printf("%ld", n);
        return;
    }

    if (n >= 100)
        printf("%ld", n/10);
    else
        printf("%ld.%ld", n/10, n%10);
    putchar(suffix);

    return;
}
