#include <stdio.h>
#include <stdlib.h>

#include "../include/inode_table.h"
#include "../include/utils.h"

int tests_failed = 0;
int tests_total = 0;
void err(int cond, char *msg) {
  ++tests_total;
  if (!cond)
    printf("%dth test failed: %s\n", ++tests_failed, msg);
}


void inode_table_tests() {
  struct fu_table_t *nodes = fu_table_alloc();
  struct fu_node_t *p = fu_table_add(nodes, 0, "/", 1);

  err(p != NULL, "cannot add root node");
  err(p == fu_table_get(nodes, 1), "cannot retrieve the root back");

  struct fu_node_t *n = fu_table_add(nodes, 1, "mono", 2);
  err(fu_node_parent(n) == p, "parent not set to root!");

  fu_table_add(nodes, 1, "news", 3);
  n = fu_table_add(nodes, 3, "more", 4);

  err(n == fu_table_lookup(nodes, 3, "more"), "cannot lookup files under a specific folder in fu_table_t !");

  struct fu_buf_t path_buf = fu_get_path(nodes, 4, NULL);

  err(strcmp(path_buf.data, "/news/more") == 0, "not getting correct path for a sub folder in fu_table_t!");

  fu_buf_free(&path_buf);

  fu_table_free(nodes);


  nodes = fu_table_alloc();

  fu_table_add(nodes, 0, "/", 1);
  fu_table_add(nodes, 1, "usr", 2);
  n = fu_table_add(nodes, 2, "lib", 3);

  err(n == fu_table_lookup(nodes, 2, "lib"), "cannot lookup correct folder in fu_table_t");
}

void util_tests() {
  // buf tests
  struct fu_buf_t tmp;
  fu_buf_init(&tmp, sizeof(char));
  char *str = "hello world!!";
  size_t len = strlen(str) + 1;

  fu_buf_push(&tmp, str, len);
  err(strcmp((char *) tmp.data, str) == 0, "items corrupted in fu_buf_t");
  err(tmp.size == len, "incorrect size in fu_buf_t");

  fu_buf_free(&tmp);

  struct fu_table_t *nodes = fu_table_alloc();
  fu_table_add(nodes, 0, "/", 1);
  struct fu_node_t *p = fu_table_add(nodes, 1, "usr", 2);

  err(p == fu_table_lookup(nodes, 1, "usr"), "cannot find home dir in fu_table_t");
  struct fu_buf_t path_buf = fu_get_path(nodes, 2, NULL);

  err(strcmp(path_buf.data, "/usr") == 0, "cannot get correct path for an inode");
  fu_buf_free(&path_buf);

  path_buf = fu_get_path(nodes, 2, "local");
  err(strcmp(path_buf.data, "/usr/local") == 0, "cannot get correct path from fu_get_path");
  fu_buf_free(&path_buf);

  fu_table_free(nodes);
}
int main() {
  inode_table_tests();
  util_tests();

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
