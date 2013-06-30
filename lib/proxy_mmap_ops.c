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

// for mmap
#include <sys/mman.h>

// for intptr_t
#include <stdint.h>

#include "../include/utils.h"

struct fu_ofile_t {
  char * path;
  int fd;
};

struct fu_buf_t open_files;

void * proxy_init(struct fuse_conn_info *conn) {
  fu_buf_init(&open_files, sizeof(struct fu_ofile_t *));
  return NULL;
}

/* #include "../include/fu_ops.h" */

/* standard fs operations, just mapping them to the glibc calls */
int proxy_getattr(const char *path, struct stat *sbuf) {

  if (lstat(path, sbuf) == -1) {
    return -errno;
  }

  return 0;
}

int proxy_access(const char *path, int mask) {

	if (access(path, mask) == -1)
		return -errno;

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

  struct fu_ofile_t **files = open_files.data;
  struct fu_ofile_t *file = NULL;
  int num_files = open_files.size / sizeof(struct fu_ofile_t *), i = 0;


  for (i = 0; i < num_files; i++) {
    if (strcmp(files[i]->path, path) == 0) {
      file = files[i];
      break;
    }
  }

  if (!file) {
    int fd = open(path, finfo->flags);
    if (fd == -1) {
      perror("cannot open file\n");
      return -errno;
    }

    printf("creating a new file (%s) with fd: %d\n", path, fd);

    file = malloc(sizeof(struct fu_ofile_t));
    file->fd = fd;
    file->path = malloc(sizeof(char) * strlen(path));
    strcpy(file->path, path);
    fu_buf_push(&open_files, &file, sizeof(struct fu_ofile_t *));
  }

  finfo->fh = i;

  return 0;

}

/* TODO: handle direct io params in proxy_read and proxy_write */
int proxy_read(const char *path, char *buf, size_t count, off_t offset, struct fuse_file_info *finfo) {
  struct fu_ofile_t **files = open_files.data;
  struct fu_ofile_t *file = files[finfo->fh];

  int res = pread(file->fd, buf, count, offset);

  if (res == -1) {
    return -errno;
  }

  return res;

}



// zero copy reads through pipe fds and slice trick
int proxy_read_buf(const char *path, struct fuse_bufvec **bufp, size_t size, off_t offset, struct fuse_file_info *finfo) {
  struct fu_ofile_t **files = open_files.data;
  struct fu_ofile_t *file = files[finfo->fh];

  *bufp = malloc(sizeof(struct fuse_bufvec));
  **bufp = FUSE_BUFVEC_INIT(size);

  (*bufp)->buf[0].flags = FUSE_BUF_IS_FD | FUSE_BUF_FD_SEEK;
  (*bufp)->buf[0].fd = file->fd;
  (*bufp)->buf[0].pos = offset;

  return 0;
}

static uint64_t getfh(DIR *dh) {
  // for gcc pointer integer size mismatch warning
  return (uint64_t) (intptr_t) dh;
}

static DIR * getdh(uint64_t fh) {
  // for gcc pointer integer size mismatch warning
  return (DIR *) (intptr_t) fh;
}

int proxy_opendir(const char *path, struct fuse_file_info *finfo) {
  DIR *dh = opendir(path);

  // hack to store the dir pointer
  finfo->fh = getfh(dh);

  if (dh == NULL) {
    return -errno;
  }

  return 0;
}

int proxy_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *finfo) {
  DIR *dh = getdh(finfo->fh);
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
  DIR *dh = getdh(finfo->fh);

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

  return 0;
}

struct fuse_operations get_ops() {
  struct fuse_operations proxy_operations = {
    .init       = proxy_init,

    .getattr    = proxy_getattr,
    .access     = proxy_access,
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
