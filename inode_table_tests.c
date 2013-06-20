#include <stdio.h>
#include <stdlib.h>

#include "inode_table.h"

int tests_failed = 0;
int tests_total = 0;
void err(int cond, char *msg) {
  ++tests_total;
  if (!cond)
    printf("%dth test failed: %s\n", ++tests_failed, msg);
}
int main() {
  struct fu_table_t *nodes = fu_table_alloc();
  struct fu_node_t *p = fu_table_add(nodes, 0, "/", 1);

  err(p != NULL, "cannot add root node");
  err(p == fu_table_get(nodes, 1), "cannot retrieve the root back");

  struct fu_node_t *n = fu_table_add(nodes, 1, "mono", 2);
  err(fu_node_parent(n) == p, "parent not set to root!");

  fu_table_add(nodes, 1, "news", 3);
  n = fu_table_add(nodes, 1, "more", 4);

  err(n == fu_table_lookup(nodes, 1, "more"), "cannot lookup files under root!");

  fu_table_free(nodes);
  if (tests_failed == 0) {
    printf("%d out of %d tests passed! :) \n",
        tests_total-tests_failed, tests_total);
    return 0;
  }
  else {
    printf("unfortunately %d out of %d tests failed! :( \n",
        tests_failed, tests_total);
    return -1;
  }
}
