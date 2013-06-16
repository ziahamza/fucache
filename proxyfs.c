/*
 * a proxy fuse filesystem that mounts root file system on a mount point,
 */
#define FUSE_USE_VERSION 30

#include <fuse.h>
#include <stdio.h>
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
  int size = readlink(path, res, len);

  if (size == -1) {
    return -errno;
  }

  assert(size >= 0);

  // terminate the string if there is still space
  if (len - size > 0) {
    res[size] = '\0';
  }

  return 0;
}

int proxy_mknod(const char *path, mode_t mode, dev_t dev) {
  // works in linux, althogh non portable
  int res = mknod(path, mode, dev);

  if (res == -1) {
    return -errno;
  }

  return 0;
}

int proxy_mkdir(const char *path, mode_t mode) {
  int res = mkdir(path, mode);

  if (res == 1) {
    return -errno;
  }

  return 0;
}

int proxy_unlink(const char *path) {
  int res = unlink(path);

  if (res == -1) {
    return -errno;
  }

  return 0;
}

int proxy_rmdir(const char* path) {
  int res = rmdir(path);

  if (res == -1) {
    return -errno;
  }

  return 0;
}

int proxy_symlink(const char *fpath, const char *spath) {
  int res = symlink(fpath, spath);

  if (res == -1) {
    return -errno;
  }

  return 0;
}

int proxy_rename(const char *opath, const char *npath) {
  int res = rename(opath, npath);

  if (res == -1) {
    return -errno;
  }

  return 0;
}

int proxy_link(const char *fpath, const char *lpath) {
  int res = link(fpath, lpath);

  if (res == -1) {
    return -errno;
  }

  return 0;
}

int proxy_chmod(const char *path, mode_t mode) {
  int res = chmod(path, mode);

  if (res == -1) {
    return -errno;
  }

  return 0;
}

int proxy_chown(const char *path, uid_t owner, gid_t group) {
  int res = lchown(path, owner, group);

  if (res == -1) {
    return -errno;
  }

  return 0;
}

int proxy_truncate(const char *path, off_t length) {
  int res = truncate(path, length);

  if (res == -1) {
    return -errno;
  }

  return 0;
}

int proxy_open(const char *path, struct fuse_file_info *finfo) {
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

int proxy_write(const char *path, const char *buf, size_t count, off_t offset, struct fuse_file_info *finfo) {
  // file handle valid
  assert(finfo->fh >= 0);

  int res = pwrite(finfo->fh, buf, count, offset);

  if (res == -1) {
    return -errno;
  }

  return res;
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

int proxy_fsync(const char *path, int isdatasync, struct fuse_file_info *finfo) {
  // file handle valid
  assert(finfo->fh >= 0);

  int res;
  if (isdatasync) {
    res = fdatasync(finfo->fh);
  }
  else {
    res = fsync(finfo->fh);
  }

  if (res == -1) {
    return -errno;
  }

  return 0;
}

struct fuse_operations proxy_operations = {
  .getattr    = proxy_getattr,
  .readlink   = proxy_readlink,
  .mknod      = proxy_mknod,
  .mkdir      = proxy_mkdir,
  .unlink     = proxy_unlink,
  .rmdir      = proxy_rmdir,
  .symlink    = proxy_symlink,
  .rename     = proxy_rename,
  .link       = proxy_link,
  .chmod      = proxy_chmod,
  .chown      = proxy_chown,
  .truncate   = proxy_truncate,
  .open       = proxy_open,
  .read       = proxy_read,
  .write      = proxy_write,
  .release    = proxy_release,
  .fsync      = proxy_fsync,

  .opendir    = proxy_opendir,
  .readdir    = proxy_readdir,
  .releasedir = proxy_releasedir
};

int main(int argc, char *argv[]) {
	return fuse_main(argc, argv, &proxy_operations, NULL);
}
