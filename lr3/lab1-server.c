// lab1-server.c
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <errno.h>
#include <ctype.h>
#include <string.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <semaphore.h>
#include <unistd.h>

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

static void write_response(shm_block_t *blk, const char *s) {
    size_t len = strlen(s);
    if (len > RESP_MAX) len = RESP_MAX;
    memcpy(blk->resp, s, len);
    blk->resp_len = len;
}

int main(int argc, char **argv) {
    if (argc != 4) {
        fprintf(stderr, "usage: %s <shm_name> <sem_req_name> <sem_resp_name>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    const char *shm_name = argv[1];
    const char *sem_req_name = argv[2];
    const char *sem_resp_name = argv[3];

    int shm_fd = shm_open(shm_name, O_RDWR, 0600);
    if (shm_fd == -1) die("shm_open(open)");

    shm_block_t *blk = mmap(NULL, sizeof(shm_block_t),
                            PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (blk == MAP_FAILED) die("mmap");
    close(shm_fd);

    sem_t *sem_req = sem_open(sem_req_name, 0);
    if (sem_req == SEM_FAILED) die("sem_open(sem_req)");

    sem_t *sem_resp = sem_open(sem_resp_name, 0);
    if (sem_resp == SEM_FAILED) die("sem_open(sem_resp)");

    for (;;) {
        while (sem_wait(sem_req) == -1) {
            if (errno == EINTR) continue;
            die("sem_wait(sem_req)");
        }

        if (blk->req_len == 0) {
            break;
        }

        blk->req[REQ_MAX - 1] = '\0';

        char *p = blk->req;
        char *end;
        long v;

        while (isspace((unsigned char)*p)) ++p;

        errno = 0;
        v = strtol(p, &end, 10);
        if (p == end) {
            blk->resp_len = 0;
            blk->resp[0] = '\0';
            blk->server_exit_code = 0;
            sem_post(sem_resp);
            continue;
        }
        if (errno == ERANGE || v < INT_MIN || v > INT_MAX) {
            fprintf(stderr, "value out of int range\n");
            blk->resp_len = 0;
            blk->resp[0] = '\0';
            blk->server_exit_code = 0;
            sem_post(sem_resp);
            continue;
        }

        int res = (int)v;
        p = end;

        int fatal = 0;

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
                write_response(blk, "division by zero\n");
                blk->server_exit_code = 2;
                sem_post(sem_resp);
                fatal = 1;
                break;
            }

            res /= (int)v;
            p = end;
        }

        if (fatal) {
            munmap(blk, sizeof(*blk));
            sem_close(sem_req);
            sem_close(sem_resp);
            exit(2);
        }

        char out[64];
        snprintf(out, sizeof(out), "%d\n", res);
        write_response(blk, out);
        blk->server_exit_code = 0;

        if (sem_post(sem_resp) == -1) die("sem_post(sem_resp)");
    }

    munmap(blk, sizeof(*blk));
    sem_close(sem_req);
    sem_close(sem_resp);

    exit(EXIT_SUCCESS);
}
