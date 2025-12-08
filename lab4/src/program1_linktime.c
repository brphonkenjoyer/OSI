#include "contracts.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void print_help(void) {
    puts("Program #1 (link-time): uses one implementation chosen at compile/link time.");
    puts("Commands:");
    puts("  1 x                 -> convert(x) and print result");
    puts("  2 n a1 a2 ... an    -> sort array and print sorted numbers");
    puts("  q                   -> quit");
}

int main(void) {
    print_help();

    char line[4096];
    while (1) {
        printf("> ");
        if (!fgets(line, sizeof(line), stdin)) break;

        // trim leading spaces
        char *p = line;
        while (*p == ' ' || *p == '\t') p++;

        if (*p == 'q' || *p == 'Q') break;
        if (*p == '\n' || *p == '\0') continue;

        int cmd = 0;
        if (sscanf(p, "%d", &cmd) != 1) {
            puts("Bad command. Type q to quit.");
            continue;
        }

        if (cmd == 1) {
            int x;
            if (sscanf(p, "%*d %d", &x) != 1) {
                puts("Usage: 1 x");
                continue;
            }
            char* s = convert(x);
            if (!s) {
                puts("convert() failed (out of memory?)");
                continue;
            }
            printf("%s\n", s);
            free(s);
        } else if (cmd == 2) {
            // parse n then n ints from the line
            long n;
            char* cur = p;
            // skip cmd
            strtol(cur, &cur, 10);
            n = strtol(cur, &cur, 10);
            if (n <= 0 || n > 100000) {
                puts("Usage: 2 n a1 a2 ... an   (n must be 1..100000)");
                continue;
            }
            int* arr = (int*)malloc((size_t)n * sizeof(int));
            if (!arr) { puts("malloc failed"); continue; }

            int ok = 1;
            for (long i = 0; i < n; ++i) {
                char* end;
                long v = strtol(cur, &end, 10);
                if (end == cur) { ok = 0; break; }
                arr[i] = (int)v;
                cur = end;
            }
            if (!ok) {
                puts("Not enough numbers. Usage: 2 n a1 a2 ... an");
                free(arr);
                continue;
            }

            sort(arr, (size_t)n);
            for (long i = 0; i < n; ++i) {
                if (i) putchar(' ');
                printf("%d", arr[i]);
            }
            putchar('\n');
            free(arr);
        } else {
            puts("Unknown command. Use 1 or 2, or q to quit.");
        }
    }

    return 0;
}
