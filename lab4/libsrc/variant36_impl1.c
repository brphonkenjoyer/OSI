#include "contracts.h"

#include <stdlib.h>
#include <string.h>

static char* itoa_base(int x, int base) {
    // base in [2..36]
    if (base < 2 || base > 36) return NULL;

    int negative = (x < 0);
    unsigned int ux = negative ? (unsigned int)(- (long long)x) : (unsigned int)x;

    // special case 0
    if (ux == 0) {
        char* s = (char*)malloc(negative ? 3 : 2);
        if (!s) return NULL;
        if (negative) { s[0]='-'; s[1]='0'; s[2]='\0'; }
        else { s[0]='0'; s[1]='\0'; }
        return s;
    }

    char buf[64];
    size_t i = 0;
    while (ux > 0 && i < sizeof(buf)-1) {
        unsigned int digit = ux % (unsigned int)base;
        buf[i++] = (digit < 10) ? (char)('0' + digit) : (char)('a' + (digit - 10));
        ux /= (unsigned int)base;
    }

    size_t len = i + (negative ? 1 : 0);
    char* out = (char*)malloc(len + 1);
    if (!out) return NULL;

    size_t pos = 0;
    if (negative) out[pos++] = '-';
    // reverse digits
    while (i > 0) out[pos++] = buf[--i];
    out[pos] = '\0';
    return out;
}

char* convert(int x) {
    // Implementation #1: binary
    return itoa_base(x, 2);
}

int* sort(int* array, size_t n) {
    // Implementation #1: bubble sort
    if (!array || n < 2) return array;

    for (size_t i = 0; i + 1 < n; ++i) {
        int swapped = 0;
        for (size_t j = 0; j + 1 < n - i; ++j) {
            if (array[j] > array[j + 1]) {
                int tmp = array[j];
                array[j] = array[j + 1];
                array[j + 1] = tmp;
                swapped = 1;
            }
        }
        if (!swapped) break;
    }
    return array;
}
