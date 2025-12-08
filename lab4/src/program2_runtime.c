#define _POSIX_C_SOURCE 200809L
#include "contracts.h"

#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef char* (*convert_fn)(int);
typedef int*  (*sort_fn)(int*, size_t);

typedef struct {
    void* handle;
    convert_fn convert;
    sort_fn sort;
    const char* path;
} plugin_t;

static int load_plugin(plugin_t* p, const char* path) {
    memset(p, 0, sizeof(*p));
    p->path = path;

    p->handle = dlopen(path, RTLD_LAZY);
    if (!p->handle) {
        fprintf(stderr, "dlopen('%s') failed: %s\n", path, dlerror());
        return 0;
    }

    dlerror(); // clear
    p->convert = (convert_fn)dlsym(p->handle, "convert");
    const char* e1 = dlerror();
    p->sort = (sort_fn)dlsym(p->handle, "sort");
    const char* e2 = dlerror();

    if (e1 || e2 || !p->convert || !p->sort) {
        fprintf(stderr, "dlsym failed for '%s': %s %s\n",
                path, e1 ? e1 : "", e2 ? e2 : "");
        dlclose(p->handle);
        memset(p, 0, sizeof(*p));
        return 0;
    }

    return 1;
}

static void unload_plugin(plugin_t* p) {
    if (p->handle) dlclose(p->handle);
    memset(p, 0, sizeof(*p));
}

static void print_help(void) {
    puts("Program #2 (run-time): loads libraries using dlopen/dlsym.");
    puts("Commands:");
    puts("  0                   -> switch implementation (impl1 <-> impl2)");
    puts("  1 x                 -> convert(x) and print result");
    puts("  2 n a1 a2 ... an    -> sort array and print sorted numbers");
    puts("  q                   -> quit");
}

int main(void) {
    // Relative paths (as required in the task)
    const char* paths[2] = {
        "./lib/libvariant36_impl1.so",
        "./lib/libvariant36_impl2.so"
    };

    plugin_t plugin;
    int which = 0;
    if (!load_plugin(&plugin, paths[which])) {
        return 1;
    }

    print_help();
    printf("Loaded: %s\n", plugin.path);

    char line[4096];
    while (1) {
        printf("> ");
        if (!fgets(line, sizeof(line), stdin)) break;

        char *p = line;
        while (*p == ' ' || *p == '\t') p++;

        if (*p == 'q' || *p == 'Q') break;
        if (*p == '\n' || *p == '\0') continue;

        int cmd = 0;
        if (sscanf(p, "%d", &cmd) != 1) {
            puts("Bad command. Type q to quit.");
            continue;
        }

        if (cmd == 0) {
            unload_plugin(&plugin);
            which ^= 1;
            if (!load_plugin(&plugin, paths[which])) return 1;
            printf("Switched. Loaded: %s\n", plugin.path);
            continue;
        }

        if (cmd == 1) {
            int x;
            if (sscanf(p, "%*d %d", &x) != 1) {
                puts("Usage: 1 x");
                continue;
            }
            char* s = plugin.convert(x);
            if (!s) {
                puts("convert() failed (out of memory?)");
                continue;
            }
            printf("%s\n", s);
            free(s);
        } else if (cmd == 2) {
            long n;
            char* cur = p;
            strtol(cur, &cur, 10);      // cmd
            n = strtol(cur, &cur, 10);  // n

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

            plugin.sort(arr, (size_t)n);
            for (long i = 0; i < n; ++i) {
                if (i) putchar(' ');
                printf("%d", arr[i]);
            }
            putchar('\n');
            free(arr);
        } else {
            puts("Unknown command. Use 0/1/2, or q to quit.");
        }
    }

    unload_plugin(&plugin);
    return 0;
}
