#ifndef AQFS_FS_H
#define AQFS_FS_H

#define FUSE_USE_VERSION 26
#include <fuse.h>

namespace aqfs {

struct fs {
    static void *init(struct fuse_conn_info *conn);
    static void destroy(void *private_data);

    static int getattr(const char *path, struct stat *buf);
    static int readlink(const char *path, char *buf, size_t size);
    static int opendir(const char *path, struct fuse_file_info *fi);
    static int readdir(const char *path, void *buf, fuse_fill_dir_t filler,
                       off_t off, struct fuse_file_info *fi);
    static int mkdir(const char *path, mode_t mode);
    static int unlink(const char *path);
    static int rmdir(const char *path);
    static int symlink(const char *to, const char *from);
    static int rename(const char *from, const char *to);
    static int link(const char *from, const char *to);
    static int truncate(const char *path, off_t size);
    static int open(const char *path, struct fuse_file_info *fi);
    static int read(const char *path, char *buf, size_t size, off_t offset,
                    struct fuse_file_info *fi);
    static int write(const char *path, char *buf, size_t size, off_t offset,
                     struct fuse_file_info *fi);
    static int release(const char *path, struct fuse_file_info *fi);
    static int releasedir(const char *path, struct fuse_file_info *fi);

    fs() = delete;
    fs(int argc, char *argv[]) {}
    int mount() { return 0; }
};

} // namespace aqfs

#endif
