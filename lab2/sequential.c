#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

void deckInit(int *deck) {
  int cardIndex = 0;

  for (int suit = 0; suit < 4; suit++) {
    for (int rank = 0; rank < 13; rank++) {
      deck[cardIndex] = rank;
      cardIndex++;
    }
  }
}

void deckShuffle(int *deck, int size) {
  for (int i = size - 1; i > 0; i--) {
    int j = rand() % (i + 1);

    int temp = deck[i];

    deck[i] = deck[j];
    deck[j] = temp;
  }
}

long int runOneRound() {
  int deck[52];
  int deckSize = 52;

  deckInit(deck);
  deckShuffle(deck, deckSize);

  return (deck[0] == deck[1]);
}

int main(int argc, char *argv[]) {

  if (argc != 2) {
    return -1;
  }

  char *pEnd;
  long int totalRounds = strtol(argv[1], &pEnd, 10);
  if (*pEnd != '\0' || totalRounds <= 0) {
    return -2;
  }

  srand(time(NULL));

  long int successCount = 0;

  clock_t start = clock();

  for (long int i = 0; i < totalRounds; i++) {
    successCount += runOneRound();
  }

  clock_t end = clock();

  double timeSpentMs = ((double)(end - start) / CLOCKS_PER_SEC) * 1000.0;

  double probability = (double)successCount / totalRounds;

  char buffer[256];
  int len = 0;

  len += snprintf(buffer + len, sizeof(buffer) - len,
                  "кол-во успешных нахождений: %ld\n", successCount);
  len += snprintf(buffer + len, sizeof(buffer) - len, "вероятность: %f\n",
                  probability * 100);
  len += snprintf(buffer + len, sizeof(buffer) - len,
                  "время выполнения: %fms\n", timeSpentMs);

  write(1, buffer, len);

  return 0;
}