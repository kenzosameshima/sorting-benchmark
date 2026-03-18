#include <stdlib.h>
#include <time.h>

#include "tests.h"

int main(void) {
  srand((unsigned int)time(NULL));
  run_experiments();
  return 0;
}
