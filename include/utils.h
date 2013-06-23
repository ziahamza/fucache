#ifndef __UTILS_H_
#define __UTILS_H_

#include "../include/inode_table.h"

#include <stdlib.h>
#include <string.h>

struct fu_buf_t {
  void *data;
  int cap;
  int size;
};

void fu_buf_init(struct fu_buf_t *buf, int init_size);
void fu_buf_resize(struct fu_buf_t *buf, size_t size);
void fu_buf_reset(struct fu_buf_t *buf);
void fu_buf_push(struct fu_buf_t *buf, const void *item, size_t size);
void fu_buf_free(struct fu_buf_t *buf);

struct fu_buf_t get_path(struct fu_table_t *, fuse_ino_t, const char *);

#endif
