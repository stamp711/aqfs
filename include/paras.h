#ifndef AQFS_PARAS_H
#define AQFS_PARAS_H

namespace aqfs {

const int NBLKS = 1024;
const int BLKSIZE = 4096;

const int BASE_BOOT_BLK   = 0;
const int BASE_SUPER_BLK  = 1;
const int BASE_BITMAP_BLK = 2;
const int BASE_INODE_BLK  = 3;
const int BASE_DATA_BLKS  = 64;

const int INODES_PER_BLK = 64;
const int N_INODE_BLKS   = 61;
const int N_INODES       = INODES_PER_BLK * N_INODE_BLKS;
const int N_DATA_BLKS    = 960;
const int N_DBLKS        = N_DATA_BLKS;

const int DIRECT_BLKS_PER_INODE         = 5;
const int SINGLE_INDRECT_BLKS_PER_INODE = 8;
const int INDRECT_LINK_PER_BLK          = 128;

const int DIRENTRY_PER_BLK = 128;
const int MAX_FILENAME = 27; // 28 - 1

} // namespace aqfs

#endif
