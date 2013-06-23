#include <stdlib.h>

#include <fuse_lowlevel.h>

#include "../include/inode_table.h"
#include "../include/utils.h"

void fu_buf_init(struct fu_buf_t *buf, int init_size) {
  buf->cap = init_size;
  buf->data = malloc(init_size);
  buf->size = 0;
}
void fu_buf_resize(struct fu_buf_t *buf, size_t nsize) {
  buf->cap = nsize;
  buf->data = realloc(buf->data, buf->cap);
}

void fu_buf_reset(struct fu_buf_t *buf) {
  buf->size = 0;
}

void fu_buf_push(struct fu_buf_t *buf, const void *item, size_t size) {
  if (size + buf->size > buf->cap) {
    fu_buf_resize(buf, 2 * (size + buf->size));
  }

  memcpy(buf->data + buf->size, item, size);
  buf->size += size;
}

void fu_buf_free(struct fu_buf_t *buf) {
  free(buf->data);
}

// have to free the return buffer using the fu_buf_free when used
struct fu_buf_t get_path(struct fu_table_t * table, fuse_ino_t pid, const char *name) {
  size_t ptrsize = sizeof(const char *);
  // buffer of strings, should be concatinated from reverse at the end
  struct fu_buf_t tmp;
  fu_buf_init(&tmp, ptrsize * 20);

  if (name) {
    fu_buf_push(&tmp, &name, ptrsize);
  }

  for (
    struct fu_node_t *node = fu_table_get(table, pid);
    fu_node_inode(node) != FUSE_ROOT_ID;
    node = fu_node_parent(node)
  ) {
    const char *pname = fu_node_name(node);
    fu_buf_push(&tmp, &pname, ptrsize);
  }

  // generate final string
  struct fu_buf_t res;
  fu_buf_init(&res, sizeof(char) * 256);

  // array of strings
  const char ** strs = tmp.data;

  for (int ind = (tmp.size / ptrsize) - 1; ind >= 0; ind--) {
    fu_buf_push(&res, "/", 1);
    fu_buf_push(&res, strs[ind], strlen(strs[ind]));
  }

  fu_buf_push(&res, "\0", 1);

  fu_buf_free(&tmp);

  return res;
}
