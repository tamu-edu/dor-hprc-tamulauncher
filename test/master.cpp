#include <string>
#include "omp.h"

int main() {
#pragma omp parallel num_threads(2)
  {
#pragma omp for
  for (int i=0;i<4;++i) {

    char str[100];
    sprintf(str, "./slave.x  %d", omp_get_thread_num());
    system(str);
  }
}
}
