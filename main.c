#define FUSE_USE_VERSION 30
#include <fuse.h>

// for get_ops
#include "fu_ops.h"

// high level init
#include "init.c"


int main(int argc, char *argv[]) {
  // currently using high level fuse apis for fs
  struct fuse_operations ops = get_ops();
  return init(argc, argv, &ops);
}
