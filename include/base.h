#ifndef AQFS_BASE_H
#define AQFS_BASE_H

#include "paras.h"
#include <bitset>

namespace aqfs {

/* in-memory blk buffer */
template <typename T> struct blkbuf {
    uint32_t blkno;
    T data;

    blkbuf<T>() = delete;
    blkbuf<T>(uint32_t blkno) { this->blkno = blkno; }
    int fill();
    int persist();
};

/* the in-memory super block controller */
struct super_t {
    uint32_t magic;
    uint32_t clean;

    int load();
    int persist();
};

/* 0 should be reserved */
template <size_t N> class bitset : public std::bitset<N> {
  public:
    inline uint32_t find_empty() {
        for (int i = 1; i < N; i++) {
            if (this->test(i) == false)
                return i;
        }
        return 0;
    }
};

/* the in-memory bitmap blk controller */
struct bitmap_t {
    bitset<N_INODES> imap;
    bitset<N_DBLKS> dmap;

    int load();    /* 从 bitmap block 读取数据 */
    int persist(); /* 将数据写回 bitmap block */
};

} // namespace aqfs

#endif
