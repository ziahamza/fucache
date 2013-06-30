/*
 * initializes fuse fs with low level fuse apis
 */

#define FUSE_USE_VERSION 30

#include <stdio.h>
#include <stdlib.h>
#include <fuse.h>
#include <fuse_lowlevel.h>

// for intptr_t
#include <stdint.h>

// for ENOENT
#include <errno.h>

#include <sys/stat.h>

// for PATH_MAX
#include <limits.h>

// for chdir
#include <unistd.h>

// inode tables to map inodes to paths
#include "../include/inode_table.h"

// utils used for path funcs
#include "../include/utils.h"

// in case cannot find inode for an entry or calculating inode not necessary
#define FUSE_UNKNOWN_INO 0xffffffff

struct fu_ll_ctx {
  struct fu_table_t *table;
  struct fuse_operations *ops;
};
void fu_ll_lookup(fuse_req_t req, fuse_ino_t parent, const char *name) {
  printf("\n\n Inside fu_ll_lookup:\n");
  struct fu_ll_ctx *ctx = fuse_req_userdata(req);


  if (name[0] == '.' && name[1] == '.' && strlen(name) == 2) {
    parent = fu_node_inode(fu_node_parent(fu_table_get(ctx->table, parent)));
  }

  struct fu_node_t *node = fu_table_lookup(ctx->table, parent, name);
  if (!node) {
    struct stat st = {0};

    struct fu_buf_t path_buf = fu_get_path(ctx->table, parent, name);
    const char *path = path_buf.data;
    int res = ctx->ops->getattr(path, &st);

    printf("node not found, trying to add it!! path: %s\n", path);

    fu_buf_free(&path_buf);

    if (res != 0) {

      printf("looking up and error, path: %s\n", path);
      fuse_reply_err(req, -res);
      return;
    }

    node = fu_table_add(ctx->table, parent, name, st.st_ino);
    fu_node_setstat(node, st);

  }
  else {
    printf("getting node %s from cache!\n", name);
  }


  // reply entry for lookup
  struct fuse_entry_param e = {0};
  e.generation = 0;
  e.entry_timeout = 3500;
  e.attr_timeout = 3500;
  e.ino = fu_node_inode(node);
  e.attr = fu_node_stat(node);


  fuse_reply_entry(req, &e);

}

void fu_ll_getattr(fuse_req_t req, fuse_ino_t ino, struct fuse_file_info *fi) {
  printf("\n\nInside fu_ll_getattr:\n");
  struct fu_ll_ctx *ctx = fuse_req_userdata(req);
  struct stat stbuf = {0};

  struct fu_node_t *node = fu_table_get(ctx->table, ino);
  if (node) {
    printf("reading %s from cache!\n", fu_node_name(node));
    stbuf = fu_node_stat(node);

    fuse_reply_attr(req, &stbuf, 3500);
    return;
  }

  struct fu_buf_t path_buf = fu_get_path(ctx->table, ino, NULL);

  const char *path = path_buf.data;

  int res = ctx->ops->getattr(path, &stbuf);
  if (res == 0) {
    printf("getting attr and success!! : %s with ino %ld \n", path, ino);
    fuse_reply_attr(req, &stbuf, 1.0);
  }
  else {
    printf("getting attr and error : %s with ino %ld \n", path, ino);
    fuse_reply_err(req, -res);
  }

  fu_buf_free(&path_buf);
}

// custom directory handle
struct fu_dh_t {
  struct fu_buf_t contents;
  struct fu_buf_t path_buf;
  int filled;
  fuse_ino_t inode;
  // fuse request for the file handle
  fuse_req_t req;
  // high level operations file handle
  uint64_t dh;
};

void fu_dh_free(struct fu_dh_t *dh) {
  fu_buf_free(&dh->contents);
  fu_buf_free(&dh->path_buf);
  free(dh);
}

struct fu_dh_t * fu_ll_getdh(uint64_t fh) {
  // for gcc pointer integer size mismatch warning
  return (struct fu_dh_t *) (intptr_t)fh;
}

uint64_t fu_ll_getfh(struct fu_dh_t * dh) {
  // for gcc pointer integer size mismatch warning
  return (uint64_t) (intptr_t)dh;
}

void fu_ll_opendir(fuse_req_t req, fuse_ino_t inode,
     struct fuse_file_info *llfi) {
  printf("\n\nInside opendir:\n");
  struct fu_ll_ctx *ctx = fuse_req_userdata(req);

  struct fu_dh_t *dh_ll = malloc(sizeof(struct fu_dh_t));
  fu_buf_init(&dh_ll->contents, 256);

  dh_ll->filled = 0;
  dh_ll->inode = inode;

  dh_ll->path_buf  = fu_get_path(ctx->table, inode, NULL);
  const char *path = dh_ll->path_buf.data;

  int res = ctx->ops->opendir(path, llfi);

  if (res != 0) {
    printf("Cannot open path: %s\n", (char *)dh_ll->path_buf.data);
    fuse_reply_err(req, -res);
    fu_dh_free(dh_ll);
    return;
  }

  // overwrite the dir handle with our low level one
  dh_ll->dh = llfi->fh;
  llfi->fh = fu_ll_getfh(dh_ll);

  printf("Opened dir successfully! with path: %s\n", (char *)dh_ll->path_buf.data);
  // TODO: handle interrupted sycall in case fuse_reply_open returns error
  // release dir in that case must be called
  fuse_reply_open(req, llfi);
}

int fill_dir(void *buf, const char *dir_name, const struct stat *st, off_t off) {
  struct fu_dh_t *dh_ll = buf;
  struct fu_ll_ctx  *ctx = fuse_req_userdata(dh_ll->req);
  int newlen = 0;
  struct stat new_st = *st;

  printf("Trying to add dir with name (%s) at off (%lld)\n", dir_name, off);

  struct fu_node_t *node = fu_table_get(ctx->table, st->st_ino);
  // replace the high level inode with our own one,
  if (node) {
    new_st.st_ino = fu_node_inode(node);
    printf("got the dir in hash table, giving it our inode (%lld) \n", new_st.st_ino);
  }
  else {
    printf("could not find the dir in the hash table, strange (%lld)\n", st->st_ino);
    new_st.st_ino = FUSE_UNKNOWN_INO;
    /*
    if (fu_table_add(ctx->table, dh_ll->inode, dir_name, new_st.st_ino)) {
      printf("added dir successfully!\n");
    }
    else {
      printf("could not add dir, maybe a pid error, resetting the inode to unknown\n");
      new_st.st_ino = FUSE_UNKNOWN_INO;
    }
    */

  }

  if (off) {
    printf("got an offset, so only adding it in that length\n");
    // not filled, just add it at the right offset
    dh_ll->filled = 0;

    newlen = dh_ll->contents.size + fuse_add_direntry(
        dh_ll->req,
        dh_ll->contents.data + dh_ll->contents.size,
        dh_ll->contents.cap - dh_ll->contents.size,
        dir_name,
        &new_st, off);

    if (newlen > dh_ll->contents.cap) {
      printf("out buffer size was not enough, stopping further entries\n\n");
      // buffer not enough, stop adding new entries
      return 1;
    }
  }
  else {
    printf("no offset so appending entries :) \n");
    // no offset, so have to append, check if enough cap to add another entry
    newlen = dh_ll->contents.size + fuse_add_direntry(dh_ll->req, NULL, 0, dir_name, NULL, 0);
    if (newlen > dh_ll->contents.cap) {
      fu_buf_resize(&dh_ll->contents, newlen);
    }

    fuse_add_direntry(
        dh_ll->req,
        dh_ll->contents.data + dh_ll->contents.size,
        dh_ll->contents.cap - dh_ll->contents.size,
        dir_name,
        &new_st, newlen);
  }

  printf("updating the size  of dir buffer to %d\n\n", newlen);
  // update buffer size
  dh_ll->contents.size = newlen;

  return 0;
}

void fu_ll_readdir(fuse_req_t req, fuse_ino_t inode, size_t size,
     off_t off, struct fuse_file_info *llfi) {
  printf("\n\nInside read dir\n");
  struct fu_ll_ctx *ctx = fuse_req_userdata(req);
  struct fu_dh_t *dh_ll = fu_ll_getdh(llfi->fh);

  if (!off) {
    dh_ll->filled = 0;
  }

  printf("dir path: %s\n", (char *)dh_ll->path_buf.data);

  if (!dh_ll->filled) {
    // make sure that it has enough size to accomodate dirs
    if (size > dh_ll->contents.size) {
      fu_buf_resize(&dh_ll->contents, size);
    }

    dh_ll->filled = 1;

    // temporarily change dir handle to the high level one
    llfi->fh = dh_ll->dh;

    dh_ll->req = req;
    printf("reading data into dh with resetted buffers!\n");
    int res = ctx->ops->readdir(
        dh_ll->path_buf.data, dh_ll, fill_dir, off, llfi);

    llfi->fh = fu_ll_getfh(dh_ll);

    if (res != 0) {
      // failed to fill the buffer
      dh_ll->filled = 0;
      fuse_reply_err(req, -res);
    }
    else {
      printf("filled the data successfully into buffers!\n");
    }
  }

  // should be filled by now if no errors
  if (dh_ll->filled) {
    if (off < dh_ll->contents.size) {
      printf("dir is filled and got correct offset (%lld) with req size (%zd) against total size (%d), replying data back :)\n", off, size, dh_ll->contents.size);
      // valid offset, normalize size now
      size = off + size > dh_ll->contents.size ?
        dh_ll->contents.size - off : size;
      fuse_reply_buf(req, dh_ll->contents.data + off, size);
    }
    else {
      printf("offset (%lld) against size (%d) too far off, returning null buffer :(\n", off, dh_ll->contents.size);
      // offset too far off, return null buffer
      fuse_reply_buf(req, NULL, 0);
    }
  }
  else {
    printf("buffer not filled, just giving back the data that is there\n");
    // buffer not filled, just return what ever we have, ignore offset
    fuse_reply_buf(req, dh_ll->contents.data, dh_ll->contents.size);
  }
}


void fu_ll_releasedir(fuse_req_t req, fuse_ino_t inode,
     struct fuse_file_info *llfi) {
  printf("\n\nInside releasedir\n");
  struct fu_ll_ctx *ctx = fuse_req_userdata(req);
  struct fu_dh_t *dh_ll = fu_ll_getdh(llfi->fh);

  llfi->fh = dh_ll->dh;
  int res = ctx->ops->releasedir(dh_ll->path_buf.data, llfi);

  printf("released dir with path %s successfully!\n", (char *)dh_ll->path_buf.data);

  fu_dh_free(dh_ll);

  fuse_reply_err(req, -res);
}

void fu_ll_open(fuse_req_t req, fuse_ino_t inode, struct fuse_file_info *fi) {
  printf("\n\nInside open:\n");
  struct fu_ll_ctx *ctx = fuse_req_userdata(req);

  struct fu_buf_t path_buf = fu_get_path(ctx->table, inode, NULL);
  printf("opening file with path (%s)\n", (char *)path_buf.data);
  int res = ctx->ops->open(path_buf.data, fi);
  if (res != 0) {
    errno = -res;
    perror("got an error opening file");
    fuse_reply_err(req, -res);
  }
  else {
    printf("opened the file!\n");
    // TODO: handle the case when syscall is interupted
    fuse_reply_open(req, fi);
  }

  fu_buf_free(&path_buf);
}

void fu_ll_read(fuse_req_t req, fuse_ino_t inode, size_t size, off_t off, struct fuse_file_info *fi) {
  printf("\n\nInside read:\n");
  struct fu_ll_ctx *ctx = fuse_req_userdata(req);
  struct fuse_bufvec *buf = malloc(sizeof(struct fuse_bufvec));
  *buf = FUSE_BUFVEC_INIT(size);
  buf->buf[0].mem = malloc(size);

  struct fu_buf_t path_buf = fu_get_path(ctx->table, inode, NULL);
  const char *path = path_buf.data;

  printf("reading path (%s)\n", path);

  int res = ctx->ops->read(path, buf->buf[0].mem, size, off, fi);
  if (res < 0) {
    printf("got an error reading file\n");
    fuse_reply_err(req, -res);
  }
  else {
    printf("successfully returned back data\n");
    fuse_reply_data(req, buf, FUSE_BUF_SPLICE_MOVE);
  }

  fu_buf_free(&path_buf);
}

void fu_ll_readlink(fuse_req_t req, fuse_ino_t inode) {
  printf("\n\nInside readlink\n");
  struct fu_ll_ctx *ctx = fuse_req_userdata(req);
  char linkname[PATH_MAX + 1];
  struct fu_buf_t path_buf = fu_get_path(ctx->table, inode, NULL);

  printf("reading link with path (%s)\n", (char *)path_buf.data);

  int res = ctx->ops->readlink(path_buf.data, linkname, sizeof(linkname));

  linkname[PATH_MAX] = '\0';

  if (res != 0) {
    printf("got an error resolving link\n");
    fuse_reply_err(req, -res);
  }
  else {
    printf("returning back the actual link (%s)\n", linkname);
    fuse_reply_readlink(req, linkname);
  }

  fu_buf_free(&path_buf);
}

void fu_ll_release(fuse_req_t req, fuse_ino_t inode, struct fuse_file_info *fi) {
  struct fu_ll_ctx *ctx = fuse_req_userdata(req);
  struct fu_buf_t path_buf = fu_get_path(ctx->table, inode, NULL);
  const char *path = path_buf.data;

  int res = ctx->ops->release(path, fi);

  if (res != 0) {
    printf("got an error closing file %s\n", path);
    fuse_reply_err(req, -res);
  }
  else {
    printf("closed the file %s successfully!\n", path);
  }

  fu_buf_free(&path_buf);

}

// TODO: implement all the low level ops
struct fuse_lowlevel_ops llops = {
  .lookup = fu_ll_lookup,
  .getattr = fu_ll_getattr,

  .opendir = fu_ll_opendir,
  .readdir = fu_ll_readdir,
  .releasedir = fu_ll_releasedir,

  .open = fu_ll_open,
  .read = fu_ll_read,
  .readlink = fu_ll_readlink,
  .release = fu_ll_release
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
  struct fu_node_t *root = fu_table_add(table, 0, "/", FUSE_ROOT_ID);

  struct stat st;
  if (ops->getattr("/", &st) == 0) {
    fu_node_setstat(root, st);
  }

  struct fu_ll_ctx ctx = {
    .table = table,
    .ops = ops
  };

  struct fuse_session *se =
    fuse_lowlevel_new(&args, &llops, sizeof(llops), &ctx);

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
  res = fuse_session_loop_mt(se);

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
