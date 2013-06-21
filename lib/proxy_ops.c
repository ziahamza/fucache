/*
 * implments fu_ops.h using ops for read only proxy fs over root dir
 */

#define FUSE_USE_VERSION 30

#include <fuse.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <dirent.h>
#include <sys/time.h>

/* standard fs operations, just mapping them to the glibc calls */
int proxy_getattr(const char *path, struct stat *sbuf) {
  int res = lstat(path, sbuf);
  if (res == -1) {
    return -errno;
  }

  return 0;
}

int proxy_readlink(const char *path, char *res, size_t len) {
  int size = readlink(path, res, len - 1);

  if (size == -1) {
    return -errno;
  }

  res[size] = '\0';

  return 0;
}

int proxy_open(const char *path, struct fuse_file_info *finfo) {

	if ((finfo->flags & 3) != O_RDONLY)
		return -EACCES;

  int fd = open(path, finfo->flags);

  if (fd == -1) {
    return -errno;
  }

  finfo->fh = fd;

  return 0;
}

/* TODO: handle direct io params in proxy_read and proxy_write */
int proxy_read(const char *path, char *buf, size_t count, off_t offset, struct fuse_file_info *finfo) {
  // file handle valid
  assert(finfo->fh >= 0);

  int res = pread(finfo->fh, buf, count, offset);

  if (res == -1) {
    return -errno;
  }

  return res;

}

// zero copy reads through pipe fds and slice trick
int proxy_read_buf(const char *path, struct fuse_bufvec **bufp, size_t size, off_t offset, struct fuse_file_info *fi) {
  struct fuse_buf buf = {
    .flags = FUSE_BUF_IS_FD | FUSE_BUF_FD_SEEK,
    .fd    = fi->fh,
    .pos   = offset
  };

  *bufp = malloc(sizeof(struct fuse_bufvec));
  **bufp = FUSE_BUFVEC_INIT(size);

  (*bufp)->buf[0] = buf;

  return 0;
}

int proxy_opendir(const char *path, struct fuse_file_info *finfo) {
  DIR *dh = opendir(path);

  // hack to store the dir pointer
  finfo->fh = (uint64_t) dh;

  if (dh == NULL) {
    return -errno;
  }

  return 0;
}

int proxy_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *finfo) {
  DIR *dh = (DIR *)finfo->fh;
  struct dirent *de;

  // handle is valid
  assert(dh != NULL);

  // I think people may call readdir again to get files, as we give it back in one shot, always rewind directories
  rewinddir(dh);

  while ((de = readdir(dh)) != NULL) {
    struct stat st = {0};
    st.st_ino =  de->d_ino;
    if (filler(buf, de->d_name, &st, 0))
      break;
  }

  return 0;
}

int proxy_releasedir(const char *path, struct fuse_file_info *finfo) {
  DIR *dh = (DIR *)finfo->fh;

  // handle is valid
  assert(dh != NULL);

  int res = closedir(dh);

  if (res == -1) {
    return -errno;
  }

  return 0;
}

int proxy_statfs(const char *path, struct statvfs *stbuf) {
  int res = statvfs(path, stbuf);

  if (res == -1) {
    return -errno;
  }

  return 0;
}

/* TODO: implement flush */

int proxy_release(const char *path, struct fuse_file_info *finfo) {
  // file handle valid
  assert(finfo->fh >= 0);

  int res = close(finfo->fh);

  if (res == -1) {
    return -errno;
  }

  return 0;
}

struct fuse_operations get_ops() {
  struct fuse_operations proxy_operations = {
    .getattr    = proxy_getattr,
    .readlink   = proxy_readlink,
    .open       = proxy_open,

    .read       = proxy_read,
    .read_buf   = proxy_read_buf,

    .release    = proxy_release,
    .statfs     = proxy_statfs,

    .opendir    = proxy_opendir,
    .readdir    = proxy_readdir,
    .releasedir = proxy_releasedir
  };

  return proxy_operations;
}
