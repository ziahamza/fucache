/*
 * initializes fuse fs with high level fuse apis
 */

#define FUSE_USE_VERSION 30

#include <stdio.h>
#include <stdlib.h>
#include <fuse.h>

int init(int argc, char *argv[], struct fuse_operations *ops) {
  struct fuse_args args = FUSE_ARGS_INIT(argc, argv);
  int res, multithreaded, foreground;
  char *mountpoint;

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

  struct fuse *fuse = fuse_new(ch, &args,
      ops, sizeof(struct fuse_operations), NULL);
  if (fuse == NULL) {
    printf("error creating a new fuse fs with our operations\n");
    goto err_unmount;
  }

  res = fuse_daemonize(foreground);

  if (res == -1) {
    printf("error daemonizing the fuse filesystem\n");
    goto err_unmount;
  }

  struct fuse_session *se = fuse_get_session(fuse);
  res = fuse_set_signal_handlers(se);
  if (res == -1) {
    printf("error setting signal handdlers on the fuse session\n");
    goto err_unmount;
  }

  if (multithreaded) {
    res = fuse_loop_mt(fuse);
  }
  else {
    res = fuse_loop(fuse);
  }
  if (res == -1) {
    printf("error running the fuse loop\n");
    goto err_sig_handlers;
  }

err_sig_handlers:
  fuse_remove_signal_handlers(se);
err_unmount:
  fuse_unmount(mountpoint, ch);
  // should only be destroyed after unmounting the channel
  if (fuse) {
    fuse_destroy(fuse);
  }
err_free:
  // need to free args as fuse may internally allocate args to modify them
  fuse_opt_free_args(&args);

  free(mountpoint);

  return 0;
}
