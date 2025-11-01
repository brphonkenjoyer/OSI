// lab1-client.c
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <semaphore.h>
#include <time.h>

#define REQ_MAX 4096
#define RESP_MAX 4096

typedef struct {
    size_t req_len;
    char   req[REQ_MAX];

    size_t resp_len;
    char   resp[RESP_MAX];

    int    server_exit_code;
} shm_block_t;

static void die(const char *msg) {
    perror(msg);
    exit(EXIT_FAILURE);
}

static void make_unique_names(char *shm_name, size_t shm_sz,
                              char *sem_req_name, size_t sem_req_sz,
                              char *sem_resp_name, size_t sem_resp_sz)
{
    pid_t pid = getpid();
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);

    snprintf(shm_name, shm_sz, "/lab1_shm_%ld_%ld_%ld",
             (long)pid, (long)ts.tv_sec, (long)ts.tv_nsec);
    snprintf(sem_req_name, sem_req_sz, "/lab1_sem_req_%ld_%ld_%ld",
             (long)pid, (long)ts.tv_sec, (long)ts.tv_nsec);
    snprintf(sem_resp_name, sem_resp_sz, "/lab1_sem_resp_%ld_%ld_%ld",
             (long)pid, (long)ts.tv_sec, (long)ts.tv_nsec);
}

static void build_server_path(char *out, size_t out_sz) {
    char self[1024];
    ssize_t n = readlink("/proc/self/exe", self, sizeof(self) - 1);
    if (n == -1) die("readlink(/proc/self/exe)");
    self[n] = '\0';

    while (n > 0 && self[n] != '/') n--;
    self[n] = '\0';

    snprintf(out, out_sz, "%s/%s", self, "lab1-server");
}

int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "usage: %s <input_file>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    FILE *in = fopen(argv[1], "r");
    if (!in) die("fopen(input)");

    char shm_name[256], sem_req_name[256], sem_resp_name[256];
    make_unique_names(shm_name, sizeof(shm_name),
                      sem_req_name, sizeof(sem_req_name),
                      sem_resp_name, sizeof(sem_resp_name));

    int shm_fd = shm_open(shm_name, O_CREAT | O_EXCL | O_RDWR, 0600);
    if (shm_fd == -1) die("shm_open(create)");

    if (ftruncate(shm_fd, (off_t)sizeof(shm_block_t)) == -1) die("ftruncate");

    shm_block_t *blk = mmap(NULL, sizeof(shm_block_t),
                            PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (blk == MAP_FAILED) die("mmap");
    close(shm_fd);

    memset(blk, 0, sizeof(*blk));

    sem_t *sem_req = sem_open(sem_req_name, O_CREAT | O_EXCL, 0600, 0);
    if (sem_req == SEM_FAILED) die("sem_open(sem_req)");

    sem_t *sem_resp = sem_open(sem_resp_name, O_CREAT | O_EXCL, 0600, 0);
    if (sem_resp == SEM_FAILED) die("sem_open(sem_resp)");

    pid_t pid = fork();
    if (pid < 0) die("fork");

    if (pid == 0) {
        char server_path[1100];
        build_server_path(server_path, sizeof(server_path));

        char *const args[] = {
            (char*)"lab1-server",
            shm_name,
            sem_req_name,
            sem_resp_name,
            NULL
        };

        execv(server_path, args);
        perror("execv(server)");
        _exit(127);
    }

    char line[REQ_MAX];
    int client_exit = 0;

    while (fgets(line, sizeof(line), in)) {
        size_t len = strlen(line);
        if (len >= REQ_MAX) {
            len = REQ_MAX - 1;
            line[len] = '\0';
        }

        memcpy(blk->req, line, len + 1);
        blk->req_len = len;
        blk->server_exit_code = 0;
        blk->resp_len = 0;
        blk->resp[0] = '\0';

        if (sem_post(sem_req) == -1) die("sem_post(sem_req)");

        while (sem_wait(sem_resp) == -1) {
            if (errno == EINTR) continue;
            die("sem_wait(sem_resp)");
        }

        if (blk->resp_len > 0) {
            ssize_t w = write(STDOUT_FILENO, blk->resp, blk->resp_len);
            (void)w;
        }

        if (blk->server_exit_code != 0) {
            client_exit = blk->server_exit_code;
            break;
        }
    }

    fclose(in);

    if (client_exit == 0) {
        blk->req_len = 0;
        blk->req[0] = '\0';
        if (sem_post(sem_req) == -1) die("sem_post(sem_req terminate)");
    }

    int status = 0;
    if (waitpid(pid, &status, 0) == -1) die("waitpid");

    munmap(blk, sizeof(*blk));

    sem_close(sem_req);
    sem_close(sem_resp);

    sem_unlink(sem_req_name);
    sem_unlink(sem_resp_name);
    shm_unlink(shm_name);

    if (client_exit != 0) {
        exit(client_exit);
    }

    if (WIFEXITED(status)) {
        int code = WEXITSTATUS(status);
        exit(code);
    } else if (WIFSIGNALED(status)) {
        fprintf(stderr, "server terminated by signal %d\n", WTERMSIG(status));
        exit(128 + WTERMSIG(status));
    }

    exit(EXIT_SUCCESS);
}
