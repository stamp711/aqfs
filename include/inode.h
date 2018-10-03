#ifndef AQFS_INODE_H
#define AQFS_INODE_H

#include "base.h"
#include <stdint.h>
#include <sys/stat.h>
#include <unistd.h>

namespace aqfs {

/* the on disk inode structure */
struct inode {
    /* metadata */
    mode_t mode;       /* file mode, see man 2 stat */
    uint32_t refcount; /* how many dirs link to this inode */
    uint32_t size;     /* file size */
    /* direct links, each point to a data block */
    uint32_t direct[DIRECT_BLKS_PER_INODE];
    /* single indirect block, each point to an indirect data block */
    uint32_t single_indrect[SINGLE_INDRECT_BLKS_PER_INODE];

    int save_to_ino(uint32_t ino);
};

/* inode block, only contain inodes */
struct inode_blk {
    struct inode inodes[INODES_PER_BLK];
};
typedef blkbuf<inode_blk> inode_blk_buf;

/* indirect data block */
struct indirect_blk {
    uint32_t link[INDRECT_LINK_PER_BLK];
};
typedef blkbuf<indirect_blk> indirect_blk_buf;

/* structure for plain file data blk */
struct data_blk {
    char content[BLKSIZE];
};
typedef blkbuf<data_blk> data_blk_buf;

/* the in memory inode_t */
class inode_t {
  private:
    uint32_t ino; /* unique inode number */
    struct inode inode;
    bool dirty;

  public:
    inode_t() = delete;
    inode_t(uint32_t ino) {
        this->setino(ino);
        this->fill();
        this->dirty = false;
    }
    ~inode_t() {
        if (this->dirty)
            this->persist();
    }

    void setino(uint32_t ino) { this->ino = ino; } /* !! set inode number !! */

    void zero() {
        this->inode = {};
        this->dirty = true;
    }

    /* set & get inode contents */
    uint32_t getino() { return this->ino; };
    mode_t getmode() { return this->inode.mode; }
    uint32_t getsize() { return this->inode.size; }
    uint32_t getrefcount() { return this->inode.refcount; }

    void setmode(mode_t mode) {
        this->inode.mode = mode;
        this->dirty      = true;
    }

    void addref() {
        this->inode.refcount++;
        this->dirty = true;
    }

    void deref() {
        this->inode.refcount--;
        this->dirty = true;
    }

    /* read nbytes from associated data, starting from offset */
    int read(size_t nbyte, off_t offset, char *buf);
    int write(size_t nbyte, off_t offset, const char *buf);
    int shrinkto(size_t nbyte);

    /**
     * If the inode structure is in memory, we may need to write changes
     * back to the on-disk inode (locate using self->ino).
     */
    int persist() {
        this->dirty = false;

        int res = this->inode.save_to_ino(this->ino);
        if (res != 0)
            this->dirty = true;
        return res;
    }

  private:
    /* fill content from inode with number `ino` on disk */
    int fill();

    /* get the file's nth data block number on the block device */
    inline uint32_t get_blkno_by_offst(off_t offset) {
        return get_blkno(offset / BLKSIZE);
    }
    uint32_t get_blkno(size_t n);
};

} // namespace aqfs

#endif
