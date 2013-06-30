/* uses the standard fuse main loop for init, used only for testing */
#define FUSE_USE_VERSION 30

#include <fuse.h>

int init(int argc, char *argv[], struct fuse_operations *ops) {
  return fuse_main(argc, argv, ops, NULL);
}
