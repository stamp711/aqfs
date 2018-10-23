#pragma once

#include "inode.h"
#include <queue>

namespace aqfs {

struct direntry {
    uint32_t ino;
    char name[28];
};

struct dir_blk {
    struct direntry entry[DIRENTRY_PER_BLK];
};

struct dir_t : public inode_t {

  public:
    dir_t(uint32_t ino) : inode_t(ino) {}

    uint32_t lookup(const char *name);
    std::queue<struct direntry> read();
    int add(uint32_t ino, const char *name);
    int remove(const char *name);
    bool hasChild();
};

} // namespace aqfs
