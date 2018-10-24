#include "inode.h"
#include "cstring"
#include "runtime.h"
#define MIN(a, b) ((a < b) ? a : b)

namespace aqfs {

int inode::save_to_ino(uint32_t ino) {
    /* 计算 block 编号 和内部字节偏移 */
    int blkno = BASE_INODE_BLK + ino / INODES_PER_BLK;
    int blkpos = (ino % INODES_PER_BLK) * sizeof(struct inode);
    /* 从 block 中读取数据到 buf */
    char buf[BLKSIZE];
    int res = Runtime::disk.read(blkno, buf);
    if (res != 0)
        return -1;
    /* 将 inode 信息写入 */
    std::memcpy(buf + blkpos, this, sizeof(struct inode));
    /* 将 buf 写回到 block */
    res = Runtime::disk.write(blkno, buf);
    if (res != 0)
        return -1;
    return 0;
}

int inode_t::fill() {
    /* 计算 block 编号和内部字节偏移 */
    int blkno = BASE_INODE_BLK + this->ino / INODES_PER_BLK;
    int blkpos = (this->ino % INODES_PER_BLK) * sizeof(struct inode);
    /* 从 block 中读取数据 */
    char buf[BLKSIZE];
    int res = Runtime::disk.read(blkno, buf);
    if (res != 0)
        return res;
    /* 拷贝相应位置的数据到 struct inode */
    std::memcpy(&this->inode, buf + blkpos, sizeof(struct inode));
    return 0;
}

/*
 * 找到 inode 连接的第 n 个 block 的编号。
 * 如果编号为 0 且 alloc 为真，那么初始化一个新的 block。
 * TODO: 必要时 free 掉 indirect blk
 */
uint32_t inode_t::blk_walk(size_t n, bool alloc, bool free) {
    uint32_t *blkno;
    blkbuf_t indirect(0);
    if (n > (this->inode.size - 1) / BLKSIZE)
        return 0;
    if (n >= DIRECT_BLKS_PER_INODE +
                 SINGLE_INDRECT_BLKS_PER_INODE * INDRECT_LINK_PER_BLK)
        return 0;
    if (n < DIRECT_BLKS_PER_INODE)
        blkno = &this->inode.direct[n];
    else {
        uint32_t *indrect_blkno;
        size_t idx_for_indirect_blk =
            (n - DIRECT_BLKS_PER_INODE) / INDRECT_LINK_PER_BLK;
        size_t idx_in_indirect_blk =
            (n - DIRECT_BLKS_PER_INODE) % INDRECT_LINK_PER_BLK;

        indrect_blkno = &this->inode.single_indrect[idx_for_indirect_blk];

        // 如果 indirect block 并没有被 allocate
        if (*indrect_blkno == 0) {
            if (!alloc)
                return 0;
            *indrect_blkno = Runtime::bitmap.dmap.find_empty();
            if (*indrect_blkno == 0)
                return 0;
            Runtime::bitmap.dmap.set(*indrect_blkno);
            this->dirty = true;
            indirect = blkbuf_t(*indrect_blkno);
            memset(&indirect.data, 0, BLKSIZE);
        }
        // 从磁盘读取 indirect block
        indirect = blkbuf_t(*indrect_blkno);
        indirect.fill();
        blkno = (uint32_t *)(&indirect.data) + idx_in_indirect_blk;
    }

    if (alloc && *blkno == 0) {
        *blkno = Runtime::bitmap.dmap.find_empty();
        if (*blkno == 0)
            return 0;
        Runtime::bitmap.dmap.set(*blkno);
        // changed link in indirect blk or inode, need to flush changes
        if (indirect.blkno != 0)
            indirect.persist();
        else
            this->dirty = true;
        // initialize an empty data block
        blkbuf_t blkbuf(*blkno);
        memset(&blkbuf.data, 0, BLKSIZE);
        blkbuf.persist();
    }

    if (free && *blkno != 0) {
        Runtime::bitmap.dmap.reset(*blkno);
        *blkno = 0;
        // 为直接连接，在 inode 中
        if (indirect.blkno == 0)
            this->dirty = true;
        // 为间接连接，在 indirect 中
        else
            indirect.persist();
    }

    return *blkno;
}

int inode_t::get_blk(size_t n, blkbuf_t *blkbuf) {
    uint32_t blkno = this->blk_walk(n, true);
    if (blkno == 0)
        return -1;
    blkbuf->blkno = blkno;
    return blkbuf->fill();
}

int inode_t::read(size_t nbyte, size_t offset, char *buf) {
    if (offset >= this->inode.size)
        return 0;
    nbyte = MIN(nbyte, this->inode.size - offset);
    blkbuf_t blkbuf;
    for (size_t pos = offset; pos < offset + nbyte;) {
        if (this->get_blk(pos / BLKSIZE, &blkbuf) != 0)
            return -1;
        int bn = MIN(BLKSIZE - pos % BLKSIZE, offset + nbyte - pos);
        memcpy(buf, &blkbuf.data + pos % BLKSIZE, bn);
        pos += bn;
        buf += bn;
    }
    return nbyte;
}

int inode_t::write(size_t nbyte, size_t offset, const char *buf) {
    // Extend file if necessary
    if (offset + nbyte > this->inode.size) {
        this->inode.size = offset + nbyte;
        this->dirty = true;
    }
    blkbuf_t blkbuf;
    for (size_t pos = offset; pos < offset + nbyte;) {
        if (this->get_blk(pos / BLKSIZE, &blkbuf) != 0)
            return -1;
        int bn = MIN(BLKSIZE - (pos % BLKSIZE), offset + nbyte - pos);
        memcpy(blkbuf.data + (pos % BLKSIZE), buf, bn);
        blkbuf.persist();
        pos += bn;
        buf += bn;
    }
    return nbyte;
}

int inode_t::extendto(size_t nbyte) {
    if (nbyte > this->inode.size) {
        this->dirty = 1;
        this->inode.size = nbyte;
    }
    return 0;
}

/*
 * 缩小文件大小
 * 当 nbyte >= 当前 inode 大小时，什么都不做
 */
int inode_t::shrinkto(size_t nbyte) {
    if (nbyte >= this->inode.size)
        return 0;
    int old_nblocks = (this->inode.size + BLKSIZE - 1) / BLKSIZE;
    int new_nblocks = (nbyte + BLKSIZE - 1) / BLKSIZE;
    for (int bno = new_nblocks; bno < old_nblocks; bno++)
        this->blk_walk(bno, false, true);

    if (new_nblocks <= DIRECT_BLKS_PER_INODE) {
        memset(this->inode.single_indrect, 0,
               sizeof(uint32_t) * SINGLE_INDRECT_BLKS_PER_INODE);
        this->dirty = true;
    }
    this->inode.size = nbyte;
    this->dirty = true;
    return 0;
}

} // namespace aqfs
