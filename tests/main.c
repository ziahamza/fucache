#include <stdio.h>

#include "tests.h"
#include "util_tests.c"
#include "inode_table_tests.c"

int main() {
  inode_table_tests();
  util_tests();

  if (get_failed() == 0) {
    printf("%d out of %d tests passed! :) \n",
        get_passed(), get_total());
    return 0;
  }
  else {
    printf("unfortunately %d out of %d tests failed! :( \n",
        get_failed(), get_total());
    return -1;
  }
}
