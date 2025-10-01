#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <errno.h>
#include <ctype.h>

int main(void) {
    char line[4096];

    while (fgets(line, sizeof line, stdin)) {
        char *p = line, *end;
        long v;

        while (isspace((unsigned char)*p)) ++p;

        errno = 0;
        v = strtol(p, &end, 10);
        if (p == end) {
            continue;
        }
        if (errno == ERANGE || v < INT_MIN || v > INT_MAX) {
            fprintf(stderr, "value out of int range\n");
            continue;
        }
        int res = (int)v;
        p = end;

        for (;;) {
            while (isspace((unsigned char)*p)) ++p;
            if (*p == '\0' || *p == '\n') break;

            errno = 0;
            v = strtol(p, &end, 10);
            if (p == end) break;
            if (errno == ERANGE || v < INT_MIN || v > INT_MAX) {
                fprintf(stderr, "value out of int range\n");
                break;
            }

            if (v == 0) {
                printf("division by zero\n");
                fflush(stdout);
                exit(2);
            }

            res /= (int)v;
            p = end;
        }

        printf("%d\n", res);
        fflush(stdout);
    }

    exit(EXIT_SUCCESS);
}


