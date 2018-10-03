#ifndef AQFS_DIR_H
#define AQFS_DIR_H

#include "inode.h"
#include <queue>

namespace aqfs {

struct direntry {
    uint32_t ino;
    char name[28];
};

struct dir_blk {
    struct direntry entry[MAX_DIRENTRY_PER_BLK];
};

struct dir_t {
    inode_t inode;

  private:
    uint32_t n_entry;
    bool n_entry_valid;

  public:
    dir_t() = delete;
    dir_t(uint32_t ino) : inode(ino) {}
    ~dir_t() {}

    size_t getentcount() {
        if (!this->n_entry_valid) {
            this->update_n_entry();
            this->n_entry_valid = true;
        }
        return n_entry;
    }
    uint32_t lookup(const char *name);
    std::queue<struct direntry> read();
    int add(uint32_t ino, const char *name);
    int remove(const char *name);

  private:
    int update_n_entry();
};

} // namespace aqfs

#endif
