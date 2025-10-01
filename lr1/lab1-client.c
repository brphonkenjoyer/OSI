#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/wait.h>

int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "usage: %s <input_file>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    FILE *in = fopen(argv[1], "r");
    if (!in) { perror("fopen(input)"); exit(EXIT_FAILURE); }

    int p2c[2], c2p[2];
    if (pipe(p2c) == -1) { perror("pipe p2c"); fclose(in); exit(EXIT_FAILURE); }
    if (pipe(c2p) == -1) { perror("pipe c2p"); fclose(in); close(p2c[0]); close(p2c[1]); exit(EXIT_FAILURE); }

    pid_t pid = fork();
    if (pid < 0) {
        perror("fork");
        fclose(in);
        close(p2c[0]); close(p2c[1]);
        close(c2p[0]); close(c2p[1]);
        exit(EXIT_FAILURE);
    }

    if (pid == 0) {
        if (dup2(p2c[0], STDIN_FILENO) == -1) { perror("dup2 p2c->stdin"); _exit(127); }
        if (dup2(c2p[1], STDOUT_FILENO) == -1) { perror("dup2 c2p->stdout"); _exit(127); }

        close(p2c[0]); close(p2c[1]);
        close(c2p[0]); close(c2p[1]);

        char self[1024];
        ssize_t n = readlink("/proc/self/exe", self, sizeof(self)-1);
        if (n == -1) { perror("readlink"); _exit(127); }
        self[n] = '\0';
        while (n > 0 && self[n] != '/') n--;
        self[n] = '\0';

        char server_path[1100];
        snprintf(server_path, sizeof(server_path), "%s/%s", self, "lab1-server");

        char *const args[] = { "lab1-server", NULL };
        execv(server_path, args);
        perror("execv(server)");
        _exit(127);
    }

    close(p2c[0]);
    close(c2p[1]);

    char line[4096];
    while (fgets(line, sizeof line, in)) {
        size_t len = strlen(line), off = 0;
        while (off < len) {
            ssize_t w = write(p2c[1], line + off, len - off);
            if (w == -1) {
                if (errno == EPIPE) goto stop_writing;
                perror("write p2c");
                fclose(in);
                close(p2c[1]);
                close(c2p[0]);
                { int st; waitpid(pid, &st, 0); }
                exit(EXIT_FAILURE);
            }
            off += (size_t)w;
        }
    }
stop_writing:
    fclose(in);
    close(p2c[1]);

    for (;;) {
        char buf[4096];
        ssize_t r = read(c2p[0], buf, sizeof buf);
        if (r == 0) break;
        if (r < 0) { perror("read c2p"); close(c2p[0]); { int st; waitpid(pid, &st, 0); } exit(EXIT_FAILURE); }
        size_t off = 0;
        while (off < (size_t)r) {
            ssize_t w = write(STDOUT_FILENO, buf + off, (size_t)r - off);
            if (w < 0) { perror("write stdout"); close(c2p[0]); { int st; waitpid(pid, &st, 0); } exit(EXIT_FAILURE); }
            off += (size_t)w;
        }
    }
    close(c2p[0]);

    int status = 0;
    if (waitpid(pid, &status, 0) == -1) { perror("waitpid"); exit(EXIT_FAILURE); }

    if (WIFEXITED(status)) {
        int code = WEXITSTATUS(status);
        if (code == 2) exit(2);
        exit(code);
    } else if (WIFSIGNALED(status)) {
        fprintf(stderr, "server terminated by signal %d\n", WTERMSIG(status));
        exit(128 + WTERMSIG(status));
    }
    exit(EXIT_SUCCESS);
}

