/*
 * initializes fuse fs with low level fuse apis
 */

#define FUSE_USE_VERSION 30

#include <stdio.h>
#include <stdlib.h>
#include <fuse.h>
#include <fuse_lowlevel.h>

// inode tables to map inodes to paths
#include "../include/inode_table.h"


// TODO: implement all the low level ops
struct fuse_lowlevel_ops llops = {
  .lookup = NULL,
  .getattr = NULL,
  .readdir = NULL,
  .open = NULL,
  .read = NULL
};

int init(int argc, char *argv[], struct fuse_operations *ops) {
  // TODO: get fuse args out altogether, implement custom args parser
  struct fuse_args args = FUSE_ARGS_INIT(argc, argv);

  int res, multithreaded, foreground;
  char *mountpoint;

  // TODO: replace with a custom implementation
  res = fuse_parse_cmdline(&args, &mountpoint, &multithreaded, &foreground);
  if (res == -1) {
    printf("error parsing cmdline\n");
    return -1;
  }

  struct fuse_chan *ch = fuse_mount(mountpoint, &args);
  if (!ch) {
    printf("error mounting the fs and creating its channel\n");
    goto err_free;
  }

  // TODO: use foreground
  fuse_daemonize(1);

  // setup internal tables to track inodes
  struct fu_table_t *table = fu_table_alloc();
  fu_table_add(table, 0, "/", FUSE_ROOT_ID);

  struct fuse_session *se =
    fuse_lowlevel_new(&args, &llops, sizeof(llops), table);

  if (!se) {
    printf("error creating a new low level fuse session\n");
    goto err_unmount;
  }

  res = fuse_set_signal_handlers(se);
  if (res == -1) {
    printf("error setting signal handdlers on the fuse session\n");
    goto err_session;
  }

  fuse_session_add_chan(se, ch);

  // TODO: add support for multithreaded session loop (using fuse_session_loop_mt)
  res = fuse_session_loop(se);

  fuse_session_remove_chan(ch);
  fuse_remove_signal_handlers(se);

err_session:
  fuse_session_destroy(se);
err_unmount:
  fuse_unmount(mountpoint, ch);
err_free:
  // need to free args as fuse may internally allocate args to modify them
  fuse_opt_free_args(&args);

  free(mountpoint);

  return res ? 1 : 0;
}
