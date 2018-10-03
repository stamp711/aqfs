#include "fs.h"
#include "dir.h"
#include "runtime.h"
#include <boost/filesystem.hpp>
#include <cstring>
#include <vector>

static const char blk_root[] = "/home/vagrant/fs";
typedef boost::filesystem::path path_t;
using aqfs::dir_t;
using aqfs::inode_t;

/* helpers */
int cd(dir_t &d, path_t p) {
    for (auto name : p) {
        /* 在目录查找对应 entry 的 inode number */
        uint32_t ino = d.lookup(name.c_str());

        /* 如果没有相应的 entry (lookup() 结果为 0)，返回 -ENOENT */
        if (ino == 0)
            return -ENOENT;

        /* 如果对应的 inode 不是一个 DIR，返回 -ENOTDIR */
        inode_t i(ino);
        if ((i.getmode() & S_IFDIR) != S_IFDIR)
            return -ENOTDIR;

        /* 将 d 改为下一级目录 */
        d = dir_t(ino);
    }
    return 0;
}

/**
 * get inode number from path
 * ino will not be 0
 */
int getino(path_t p, uint32_t &ino) {
    path_t parent = p.parent_path();
    path_t name   = p.filename();

    /* 验证根目录 */
    if (p.root_directory() != "/")
        return -ENOENT;
    if (p == "/")
        return -EISDIR;

    /* 找到 `path` 的上级目录 */
    dir_t d(2);
    int res = cd(d, parent.relative_path());
    if (res != 0)
        return res;

    /* 在其中查询该文件的 inode number */
    ino = d.lookup(name.c_str());
    if (ino == 0)
        return -ENOENT;

    return 0;
}

namespace aqfs {

void *fs::init(struct fuse_conn_info *conn) {
    Runtime::init(blk_root);
    return nullptr;
}
void fs::destroy(void *private_data) { Runtime::fini(); }

int fs::getattr(const char *path, struct stat *statbuf) {

    path_t p(path);
    path_t parent = p.parent_path();
    path_t name   = p.filename();

    /* 验证根目录 */
    if (p.root_directory() != "/")
        return -ENOENT;

    /* 如果 `path` 是根目录，直接填充信息 */
    if (p == "/") {
        inode_t root_inode(2);
        statbuf->st_ino   = 2;
        statbuf->st_mode  = root_inode.getmode();
        statbuf->st_nlink = root_inode.getrefcount();
        return 0;
    }

    /* 找到 `path` 的上级目录 */
    dir_t d(2);
    int res = cd(d, parent.relative_path());
    if (res != 0)
        return res;

    /* 在其中查询该文件的 inode number */
    uint32_t ino = d.lookup(name.c_str());
    if (ino == 0)
        return -ENOENT;

    /* 从相应的 inode 里读取元数据 */
    inode_t inode(ino);
    statbuf->st_ino   = ino;
    statbuf->st_mode  = inode.getmode();
    statbuf->st_nlink = inode.getrefcount();

    /* 如果它是普通文件，还需要读出其大小 */
    if (statbuf->st_nlink == S_IFREG)
        statbuf->st_size = inode.getsize();

    return 0;
}

int fs::readlink(const char *path, char *buf, size_t size) {
    path_t p(path);
    path_t parent = p.parent_path();
    path_t name   = p.filename();

    /* 验证根目录 */
    if (p.root_directory() != "/")
        return -ENOENT;

    /* 找到 `path` 的上级目录 */
    dir_t d(2);
    int res = cd(d, parent.relative_path());
    if (res != 0)
        return res;

    /* 在其中查询该 symlink 的 inode number */
    uint32_t ino = d.lookup(name.c_str());
    if (ino == 0)
        return -ENOENT;

    /* 从相应的 inode 里读取 symlink 内容到 `buf` */
    inode_t inode(ino);
    int slen = inode.getsize(); /* symlink length */
    if (size < slen)
        slen = size;
    inode.read(slen, 0, buf);

    /* 返回填充到 `buf` 的字节数 */
    return slen;
}

int fs::opendir(const char *path, struct fuse_file_info *fi) {
    path_t p(path);

    /* 验证根目录 */
    if (p.root_directory() != "/")
        return -ENOENT;

    /* 找到相应的目录 */
    dir_t d(2);
    int res = cd(d, p.relative_path());
    if (res != 0)
        return res;

    /**
     * 设置 fi->fh 为 目录对应的 inode number.
     * opendir() 会调用 addref()，releasedir() 中利用 fi->fh 来找到相应的 inode
     * 并在其上调用 deref().
     * [⚠️] 当 deref() 后，如果 refcount 为 0，inode 会自动销毁.
     */
    fi->fh = d.inode.getino();
    d.inode.addref();

    return 0;
}

int fs::readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t off,
                struct fuse_file_info *fi) {
    path_t p(path);

    /* 验证根目录 */
    if (p.root_directory() != "/")
        return -ENOENT;

    /* 找到目录 */
    dir_t d(2);
    int res = cd(d, p.relative_path());
    if (res != 0)
        return res;

    /* 读出该目录所有的 entry */
    auto entries = d.read();
    while (!entries.empty()) {
        /* 获取一个 entry 的 inode number */
        uint32_t ino = entries.front().ino;

        /* 读取这个 inode 的元数据 */
        struct stat st = {0};
        inode_t inode(ino);
        st.st_ino   = ino;
        st.st_mode  = inode.getmode();
        st.st_nlink = inode.getrefcount();
        if (st.st_nlink == S_IFREG)
            st.st_size = inode.getsize();

        /* 塞进buf，忽略`off` */
        filler(buf, entries.front().name, &st, 0);
        entries.pop();
    }

    return 0;
}

int fs::mkdir(const char *path, mode_t mode) {
    path_t p(path);
    path_t parent = p.parent_path();
    path_t name   = p.filename();

    /* 验证根目录 */
    if (p.root_directory() != "/")
        return -ENOENT;
    if (p == "/")
        return -EISDIR;

    /* 找到 `path` 的上级目录 */
    dir_t d(2);
    int res = cd(d, parent.relative_path());
    if (res != 0)
        return res;

    /* 如果 `path` 已存在，返回 -EEXIST */
    if (name == "." || name == "..")
        return -EEXIST;
    else if (d.lookup(name.c_str()) != 0)
        return -EEXIST;

    /* 找到一个未被使用的 inode */
    uint32_t ino = Runtime::bitmap.imap.find_empty();

    /* 创建 direntry */
    res = d.add(ino, name.c_str());
    if (res != 0)
        /* link 已满 */
        return -EMLINK;

    /* 创建 dir */
    Runtime::bitmap.imap.set(ino);
    dir_t dir(ino);
    dir.inode.zero();
    dir.inode.setmode(S_IFDIR | 0755);
    dir.inode.addref();

    dir.add(ino, ".");
    dir.add(d.inode.getino(), "..");

    return 0;
}

int fs::unlink(const char *path) {
    path_t p(path);
    path_t parent = p.parent_path();
    path_t name   = p.filename();

    /* 验证根目录 */
    if (p.root_directory() != "/")
        return -ENOENT;

    /* 找到 `path` 的上级目录 */
    dir_t d(2);
    int res = cd(d, parent.relative_path());
    if (res != 0)
        return res;

    /* 在其中查询该文件的 inode number */
    uint32_t ino = d.lookup(name.c_str());
    if (ino == 0)
        return -ENOENT;

    /* deref() */
    inode_t inode(ino);
    inode.deref();

    /* remove entry */
    d.remove(name.c_str());

    return 0;
}

int fs::rmdir(const char *path) {
    path_t p(path);
    path_t parent = p.parent_path();
    path_t name   = p.filename();

    /* 验证根目录 */
    if (p.root_directory() != "/")
        return -ENOENT;

    /* 验证 name */
    if (name == "/" || name == "." || name == "..")
        return -EISDIR;

    /* cd 到 `path` */
    dir_t target(2);
    int res = cd(target, p.relative_path());
    if (res != 0)
        return res;

    /* 检查 links */
    if (target.getentcount() == 2)
        return -ENOTEMPTY;

    /* remove the entry in its parent */
    dir_t parent_dir(target.inode.getino());
    cd(parent_dir, "..");
    parent_dir.remove(name.c_str());

    /* deref that dir */
    target.inode.deref();

    return 0;
}

int fs::symlink(const char *to, const char *from) {
    path_t p(from);
    path_t parent = p.parent_path();
    path_t name   = p.filename();

    /* 验证根目录 */
    if (p.root_directory() != "/")
        return -ENOENT;

    /* 找到 `path` 的上级目录 */
    dir_t d(2);
    int res = cd(d, parent.relative_path());
    if (res != 0)
        return res;

    /* 如果 `path` 已存在，返回 -EEXIST */
    if (d.lookup(name.c_str()) != 0)
        return -EEXIST;

    /* 找到一个未被使用的 inode */
    uint32_t ino = Runtime::bitmap.imap.find_empty();

    /* 创建 direntry */
    res = d.add(ino, name.c_str());
    if (res != 0)
        /* link 已满 */
        return -EMLINK;

    /* 创建 symlink 的 inode */
    Runtime::bitmap.imap.set(ino);
    inode_t symlink(ino);
    symlink.zero();
    symlink.setmode(S_IFLNK | 0755);
    symlink.addref();

    /* 将路径写入 symlink */
    symlink.write(strlen(to), 0, to);

    return 0;
}

int fs::rename(const char *from, const char *to) {
    path_t p(from);
    path_t parent = p.parent_path();
    path_t name   = p.filename();

    path_t to_p(from);
    path_t to_parent = to_p.parent_path();
    path_t to_name   = to_p.filename();

    /* 验证根目录 */
    if (p.root_directory() != "/" || to_p.root_directory() != "/")
        return -ENOENT;
    if (p == "/" || to_p == "/")
        return -EISDIR;

    /* 找到 `from` 和 `to` 的上级目录 */
    dir_t d(2), to_d(2);
    int res = cd(d, parent.relative_path());
    if (res != 0)
        return res;
    res = cd(to_d, to_parent.relative_path());
    if (res != 0)
        return res;

    /* 找到相应的 inode */
    uint32_t ino = d.lookup(name.c_str());
    if (ino == 0)
        return -ENOENT;
    inode_t inode(ino);

    /* create and remove entry */
    res = to_d.add(ino, to_name.c_str());
    if (res != 0)
        return -EMLINK;
    d.remove(name.c_str());

    return 0;
}

int fs::link(const char *from, const char *to) {
    path_t p(from);
    path_t parent = p.parent_path();
    path_t name   = p.filename();

    path_t to_p(from);
    path_t to_parent = to_p.parent_path();
    path_t to_name   = to_p.filename();

    /* 验证根目录 */
    if (p.root_directory() != "/" || to_p.root_directory() != "/")
        return -ENOENT;
    if (p == "/" || to_p == "/")
        return -EISDIR;

    /* 找到 `from` 和 `to` 的上级目录 */
    dir_t d(2), to_d(2);
    int res = cd(d, parent.relative_path());
    if (res != 0)
        return res;
    res = cd(to_d, to_parent.relative_path());
    if (res != 0)
        return res;

    /* 找到相应的 inode */
    uint32_t ino = d.lookup(name.c_str());
    if (ino == 0)
        return -ENOENT;
    inode_t inode(ino);

    /* 确认它不是一个目录 (POSIX.1) */
    if ((inode.getmode() & S_IFDIR) == S_IFDIR)
        return -EISDIR;

    /* create link and add ref */
    res = to_d.add(ino, to_name.c_str());
    if (res != 0)
        return -EMLINK;
    inode.addref();

    return 0;
}
int fs::truncate(const char *path, off_t size) {
    path_t p(path);
    uint32_t ino;

    if (size < 0)
        return -EINVAL;

    /* get that inode */
    int res = getino(p, ino);
    if (res != 0)
        return res;
    inode_t inode(ino);

    /* 根据 size 关系来 write 或 shrink */
    uint32_t curr_size = inode.getsize();
    if (curr_size == size)
        return 0;

    if (curr_size < size) {
        uint32_t diff = curr_size - size;
        std::vector<char> buf(diff, 0);
        int res = inode.write(size - curr_size, curr_size, buf.data());
        if (res != 0)
            return -EIO;
    }

    if (curr_size > size) {
        int res = inode.shrinkto(size);
        if (res != 0)
            return -EIO;
    }

    return 0;
}

int fs::open(const char *path, struct fuse_file_info *fi) {
    path_t p(path);

    uint32_t ino;
    int res = getino(p, ino);
    if (res != 0)
        return res;

    inode_t inode(ino);
    inode.addref();
    fi->fh = ino;

    return 0;
}

int fs::read(const char *path, char *buf, size_t size, off_t offset,
             struct fuse_file_info *fi) {

    path_t p(path);

    uint32_t ino;
    int res = getino(p, ino);
    if (res != 0)
        return res;

    inode_t inode(ino);
    int bytes_read = inode.read(size, offset, buf);
    if (res < 0)
        return -EIO;

    return bytes_read;
}

int fs::write(const char *path, char *buf, size_t size, off_t offset,
              struct fuse_file_info *fi) {

    path_t p(path);

    uint32_t ino;
    int res = getino(p, ino);
    if (res != 0)
        return res;

    inode_t inode(ino);
    int bytes_write = inode.write(size, offset, buf);
    if (res < 0)
        return -EIO;

    return bytes_write;
}

int fs::release(const char *path, struct fuse_file_info *fi) {
    uint32_t ino = fi->fh;
    inode_t inode(ino);
    inode.deref();
    return 0;
}
int fs::releasedir(const char *path, struct fuse_file_info *fi) {
    uint32_t ino = fi->fh;
    inode_t inode(ino);
    inode.deref();
    return 0;
}

} // namespace aqfs

int main(int argc, char *argv[]) { return 0; }
