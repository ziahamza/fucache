#include "tests.h"
#include "../include/inode_table.h"
#include "../include/utils.h"

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
