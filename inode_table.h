#ifndef __INODE_TABLE_H_
#define __INODE_TabLE_H_
/*
 * standard interface for implementations of inode table, linux api calls
 * expect paths while fuse low level api only gives you inodes, this table
 * resolves indoes to file names and paths
 */

#include <fuse_lowlevel.h>

struct fu_hash_t {
  struct fu_node_t **store;
  int size;
};


struct fu_node_t {
  fuse_ino_t inode;
  char *name;
  struct fu_node_t *parent;

  // used in the name hash table
  struct fu_node_t *next_name;

  // used in the indoe hash table
  struct fu_node_t *next_inode;
};

struct fu_table_t {
  struct fu_hash_t inode_table;
  struct fu_hash_t name_table;
};

void fu_table_init(struct fu_table_t *);

struct fu_node_t * fu_table_add(
    struct fu_table_t *, fuse_ino_t, char *, fuse_ino_t
);

struct fu_node_t * fu_table_lookup(struct fu_table_t *, fuse_ino_t, char *);
struct fu_node_t * fu_table_get(struct fu_table_t *, fuse_ino_t);


#endif
