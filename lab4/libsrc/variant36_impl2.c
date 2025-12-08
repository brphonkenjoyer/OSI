#include "contracts.h"

#include <stdlib.h>
#include <string.h>

static char* itoa_base(int x, int base) {
    if (base < 2 || base > 36) return NULL;

    int negative = (x < 0);
    unsigned int ux = negative ? (unsigned int)(- (long long)x) : (unsigned int)x;

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
    while (i > 0) out[pos++] = buf[--i];
    out[pos] = '\0';
    return out;
}

char* convert(int x) {
    // Implementation #2: ternary
    return itoa_base(x, 3);
}

static void qsort_hoare(int* a, long l, long r) {
    long i = l, j = r;
    int pivot = a[(l + r) / 2];

    while (i <= j) {
        while (a[i] < pivot) i++;
        while (a[j] > pivot) j--;
        if (i <= j) {
            int tmp = a[i];
            a[i] = a[j];
            a[j] = tmp;
            i++; j--;
        }
    }
    if (l < j) qsort_hoare(a, l, j);
    if (i < r) qsort_hoare(a, i, r);
}

int* sort(int* array, size_t n) {
    // Implementation #2: Hoare quicksort
    if (!array || n < 2) return array;
    qsort_hoare(array, 0, (long)n - 1);
    return array;
}
