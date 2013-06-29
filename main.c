#define FUSE_USE_VERSION 30

#include <fuse.h>

// implementations for get_ops and init defined at link time
#include "include/fu_ops.h"
#include "include/init.h"


int main(int argc, char *argv[]) {
  // currently using high level fuse apis for fs
  struct fuse_operations ops = get_ops();
  return init(argc, argv, &ops);
}
