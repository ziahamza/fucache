#ifndef __INODE_TABLE_H_
#define __INODE_TabLE_H_

/*
 * standard interface for implementations of inode table, linux api calls
 * expect paths while fuse low level api only gives you inodes, this table
 * resolves indoes to file names and paths
 */

#include <fuse_lowlevel.h>

struct fu_node_t;

// simple read only ops for fu_node_t
struct fu_node_t *fu_node_parent(struct fu_node_t *);
fuse_ino_t fu_node_inode(struct fu_node_t *);
const char * fu_node_name(struct fu_node_t *);
struct stat fu_node_stat(struct fu_node_t *);

struct fu_node_t * fu_node_setstat(struct fu_node_t *, struct stat st);

struct fu_table_t;

struct fu_table_t * fu_table_alloc();
void fu_table_free(struct fu_table_t *);

struct fu_node_t * fu_table_add(
    struct fu_table_t *, fuse_ino_t, const char *, fuse_ino_t
);

struct fu_node_t * fu_table_lookup(struct fu_table_t *, fuse_ino_t, const char *);
struct fu_node_t * fu_table_get(struct fu_table_t *, fuse_ino_t);


#endif
