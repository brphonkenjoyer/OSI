#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

typedef struct {
  long int roundsToRun;
  unsigned int seed;
} ThreadArgs;

void deckInit(int *deck) {
  int cardIndex = 0;

  for (int suit = 0; suit < 4; suit++) {
    for (int rank = 0; rank < 13; rank++) {
      deck[cardIndex] = rank;
      cardIndex++;
    }
  }
}

void deckShuffleTs(int *deck, int size, unsigned int *seedp) {
  for (int i = size - 1; i > 0; i--) {
    int j = rand_r(seedp) % (i + 1);

    int temp = deck[i];

    deck[i] = deck[j];
    deck[j] = temp;
  }
}

long int runOneRoundTs(unsigned int *seedp) {
  int deck[52];
  int deckSize = 52;

  deckInit(deck);
  deckShuffleTs(deck, deckSize, seedp);

  return (deck[0] == deck[1]);
}

void *monteCarloThread(void *args) {
  ThreadArgs *myArgs = (ThreadArgs *)args;

  long int rounds = myArgs->roundsToRun;
  unsigned int seed = myArgs->seed;

  long int localSuccessCount = 0;

  for (long int i = 0; i < rounds; i++) {
    localSuccessCount += runOneRoundTs(&seed);
  }

  long int *result = (long int *)malloc(sizeof(long int));
  *result = localSuccessCount;

  return (void *)result;
}

int main(int argc, char *argv[]) {
  if (argc != 3) {
    return -1;
  }

  char *pEnd;
  long int totalRounds = strtol(argv[1], &pEnd, 10);
  if (*pEnd != '\0' || totalRounds <= 0) {
    return -2;
  }

  char *pEnd1;
  long int numThreads = strtol(argv[2], &pEnd1, 10);
  if (*pEnd1 != '\0' || numThreads <= 0) {
    return -3;
  }

  long int roundsPerThread = totalRounds / numThreads;
  long int remainder = totalRounds % numThreads;

  pthread_t threads[numThreads];
  ThreadArgs threadArgs[numThreads];

  struct timespec start, end;
  clock_gettime(CLOCK_MONOTONIC, &start);

  for (long int i = 0; i < numThreads; i++) {
    long int roundsForThisThread = roundsPerThread;

    if (i == numThreads - 1) {
      roundsForThisThread += remainder;
    }

    threadArgs[i].roundsToRun = roundsForThisThread;
    threadArgs[i].seed = time(NULL) + i;

    if (pthread_create(&threads[i], NULL, monteCarloThread, &threadArgs[i]) !=
        0) {
      return -4;
    }
  }

  long int totalSuccessCount = 0;

  for (long int i = 0; i < numThreads; i++) {
    void *threadResult;

    if (pthread_join(threads[i], &threadResult) != 0) {
      return -5;
    }

    long int *partialSuccess = (long int *)threadResult;
    totalSuccessCount += *partialSuccess;

    free(threadResult);
  }

  clock_gettime(CLOCK_MONOTONIC, &end);

  double timeSpentMs = (end.tv_sec - start.tv_sec) * 1000 +
                       (end.tv_nsec - start.tv_nsec) / 1000000.0;
  double probability = (double)totalSuccessCount / totalRounds;

  char buffer[256];
  int len = 0;

  len += snprintf(buffer + len, sizeof(buffer) - len,
                  "кол-во успешных нахождений: %ld\n", totalSuccessCount);
  len += snprintf(buffer + len, sizeof(buffer) - len, "вероятность: %f\n",
                  probability * 100);
  len += snprintf(buffer + len, sizeof(buffer) - len,
                  "время выполнения: %fms\n", timeSpentMs);

  write(STDOUT_FILENO, buffer, len);

  return 0;
}