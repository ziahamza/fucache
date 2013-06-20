#ifndef __INIT_H_
#define __INIT_H_

#define FUSE_USE_VERSION 30
#include <fuse.h>

int init(int argc, char *argv[], struct fuse_operations *ops);

#endif
