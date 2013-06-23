#include "tests.h"
#include "../include/inode_table.h"
#include "../include/utils.h"

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

