#include "dir.h"

namespace aqfs {

int namecmp(const char *s, const char *t) {
    int res = strncmp(s, t, MAX_FILENAME);
    return res;
}

uint32_t dir_t::lookup(const char *name) {
    blkbuf_t dirblkbuf;
    direntry *entries = (direntry *)dirblkbuf.data;

    // DIR 所占的 size 总是 BLKSIZE 的整数倍
    // 每次读入一个 BLK 并在其中查找
    // 找到则返回其 ino
    for (size_t n = 0; (n * BLKSIZE) < this->getsize(); n++) {
        this->get_blk(n, &dirblkbuf);
        for (int i = 0; i < DIRENTRY_PER_BLK; i++)
            if (entries[i].ino == 0)
                continue;
            else if (namecmp(name, entries[i].name) == 0)
                // entry matches name
                return entries[i].ino;
    }

    // Not found, returns 0.
    return 0;
}

std::queue<struct direntry> dir_t::read() {
    blkbuf_t dirblkbuf;
    direntry *entries = (direntry *)dirblkbuf.data;
    std::queue<struct direntry> res;

    for (size_t n = 0; (n * BLKSIZE) < this->getsize(); n++) {
        this->get_blk(n, &dirblkbuf);
        for (int i = 0; i < DIRENTRY_PER_BLK; i++)
            if (entries[i].ino == 0)
                continue;
            else
                res.push(entries[i]);
    }
    // returns the queue
    return res;
}

int dir_t::add(uint32_t ino, const char *name) {
    blkbuf_t dirblkbuf;
    direntry *entries = (direntry *)dirblkbuf.data;
    direntry *entry = nullptr;

    // Make sure that name is not present.
    while (this->lookup(name) != 0) {
        this->remove(name);
    }

    // Look for an existing empty direntry
    size_t n;
    for (n = 0; !entry && (n * BLKSIZE) < this->getsize(); n++) {
        this->get_blk(n, &dirblkbuf);
        for (int i = 0; !entry && i < DIRENTRY_PER_BLK; i++)
            if (entries[i].ino == 0) {
                entry = &entries[i];
            }
    }

    // If all existing entries are occupied, extend dir size by BLKSIZE
    if (!entry) {
        this->inode.size += BLKSIZE;
        this->dirty = 1;
        this->get_blk(n, &dirblkbuf);
        if (dirblkbuf.blkno == 0)
            return -1;
        entry = &entries[0];
    }

    // Fill entry, and persist dirblk
    strncpy(entry->name, name, MAX_FILENAME);
    entry->ino = ino;
    dirblkbuf.persist();

    // On success, returns 0
    return 0;
}

// On remove, this will not call inode->deref()
int dir_t::remove(const char *name) {
    blkbuf_t dirblkbuf;
    direntry *entries = (direntry *)dirblkbuf.data;

    // DIR 所占的 size 总是 BLKSIZE 的整数倍
    // 每次读入一个 BLK 并在其中查找
    // 找到则返回其 ino
    for (size_t n = 0; (n * BLKSIZE) < this->getsize(); n++) {
        this->get_blk(n, &dirblkbuf);
        for (int i = 0; i < DIRENTRY_PER_BLK; i++)
            if (entries[i].ino == 0)
                continue;
            else if (namecmp(name, entries[i].name) == 0) {
                // entry matches name
                entries[i].ino = 0;
                memset(entries[i].name, 0, MAX_FILENAME);
                dirblkbuf.persist();
                return 0;
            }
    }

    // Not found, returns -1.
    return -1;
}

bool dir_t::hasChild() {
    blkbuf_t dirblkbuf;
    direntry *entries = (direntry *)dirblkbuf.data;
    bool hasChild = false;

    for (size_t n = 0; !hasChild && (n * BLKSIZE) < this->getsize(); n++) {
        this->get_blk(n, &dirblkbuf);
        for (int i = 0; i < DIRENTRY_PER_BLK; i++)
            if (entries[i].ino == 0)
                continue;
            else if (namecmp(".", entries[i].name) != 0 &&
                     namecmp("..", entries[i].name) != 0) {
                hasChild = true;
                break;
            }
    }

    return hasChild;
}

} // namespace aqfs
