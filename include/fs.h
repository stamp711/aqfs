#ifndef AQFS_FS_H
#define AQFS_FS_H

#define FUSE_USE_VERSION 26
#include <fuse.h>

namespace aqfs {

struct fs {

    struct fuse_operations op;

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
    static int chmod(const char *path, mode_t mode);
    static int truncate(const char *path, off_t size);
    static int open(const char *path, struct fuse_file_info *fi);
    static int create(const char *path, mode_t mode, struct fuse_file_info *fi);
    static int read(const char *path, char *buf, size_t size, off_t offset,
                    struct fuse_file_info *fi);
    static int write(const char *path, const char *buf, size_t size,
                     off_t offset, struct fuse_file_info *fi);
    static int release(const char *path, struct fuse_file_info *fi);
    static int releasedir(const char *path, struct fuse_file_info *fi);
    static int utimens(const char *path, const struct timespec tv[2]);

    fs() {
        op.init = init;
        op.destroy = destroy;
        op.getattr = getattr;
        op.readlink = readlink;
        // op.opendir = opendir;
        op.readdir = readdir;
        op.mkdir = mkdir;
        op.unlink = unlink;
        op.rmdir = rmdir;
        op.symlink = symlink;
        op.rename = rename;
        op.link = link;
        op.chmod = chmod;
        op.truncate = truncate;
        // op.open = open;
        op.create = create;
        op.read = read;
        op.write = write;
        // op.release = release;
        // op.releasedir = releasedir;
        op.utimens = utimens;
    }
};

} // namespace aqfs

#endif
