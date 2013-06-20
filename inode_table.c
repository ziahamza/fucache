#include <stdlib.h>
#include <string.h>

#include <fuse_lowlevel.h>

#include "inode_table.h"

#define FU_HT_MIN_SIZE 8192

size_t inode_hash(fuse_ino_t inode, size_t max) {
  return ((uint32_t) inode * 2654435761U) % max;
}

size_t name_hash(char *name, fuse_ino_t parent, size_t max) {
  uint64_t hash = parent;
  for (; *name; name++) {
    hash = hash * 31 + (unsigned char) *name;
  }

  return hash % max;
}
void fu_hash_init(struct fu_hash_t *ht, int size) {
  ht->size = size;
  ht->store = malloc(sizeof(struct node *) * size);
}

struct fu_node_t *fu_hash_findname(
    struct fu_hash_t *ht, fuse_ino_t pinode, char *name) {

  struct fu_node_t *node;
  size_t hash = name_hash(name, pinode, ht->size);
  for (node = ht->store[hash]; node; node = node->next_name) {
    if (node->parent->inode == pinode && name_hash(node->name, pinode, ht->size) == hash) {
      if (strcmp(name, node->name) == 0) {
        return node;
      }
    }
  }

  return NULL;
}

struct fu_node_t *fu_hash_findinode(struct fu_hash_t *ht, fuse_ino_t inode) {
  struct fu_node_t *node;
  size_t hash = inode_hash(inode, ht->size);
  for (node = ht->store[hash]; node; node = node->next_inode) {
    if (node->inode == inode) {
      return node;
    }
  }

  return NULL;
}


void fu_table_init(struct fu_table_t *table) {
  fu_hash_init(&table->inode_table, FU_HT_MIN_SIZE);
  fu_hash_init(&table->name_table, FU_HT_MIN_SIZE);
}

struct fu_node_t * fu_table_get(struct fu_table_t *table, fuse_ino_t inode) {
  return fu_hash_findinode(&table->inode_table, inode);
}

struct fu_node_t * fu_table_lookup(
    struct fu_table_t *table, fuse_ino_t pinode, char *name) {

  return fu_hash_findname(&table->name_table, pinode, name);
}

struct fu_node_t * fu_table_add(
    struct fu_table_t *table, fuse_ino_t pinode, char *name, fuse_ino_t inode) {
  struct fu_node_t *pnode = fu_table_get(table, pinode);
  if (!pnode && pnode != 0) {
    return NULL;
  }

  struct fu_node_t *node = malloc(sizeof(struct fu_node_t));
  node->inode = inode;
  node->name = malloc(sizeof(char) * strlen(name));
  strcpy(node->name, name);

  node->parent = pnode;

  size_t ihash = inode_hash(inode, table->inode_table.size);
  size_t nhash = name_hash(name, pinode, table->name_table.size);

  node->next_inode = table->inode_table.store[ihash];
  node->next_name = table->name_table.store[nhash];

  table->inode_table.store[ihash] = table->name_table.store[nhash] = node;

  return node;
}

