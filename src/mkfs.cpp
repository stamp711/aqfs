#include "base.h"
#include "dir.h"
#include "fs.h"
#include "paras.h"
#include "runtime.h"
#include <fstream>
#include <iostream>
#include <limits.h>
#include <string>

void print_paras() {
    using namespace aqfs;
    std::cout << "Block size: " << BLKSIZE << std::endl;
    std::cout << "Total blocks: " << NBLKS << std::endl;
    std::cout << std::endl;
    std::cout << "Inode link information:" << std::endl;
    std::cout << "    size of struct inode: " << sizeof(struct inode)
              << std::endl;
    std::cout << "    direct link per inode: " << DIRECT_BLKS_PER_INODE
              << std::endl;
    std::cout << "    single indirect blk link per inode: "
              << SINGLE_INDRECT_BLKS_PER_INODE << std::endl;
    std::cout << "    indirect link per indirect blk: " << INDRECT_LINK_PER_BLK
              << std::endl;
    std::cout << std::endl;
    std::cout << "Size of directory entry: " << sizeof(direntry) << std::endl;
    std::cout << "Max filename length: " << MAX_FILENAME << std::endl;
    std::cout << std::endl;
    std::cout << "Block information:" << std::endl;
    std::cout << "    boot on block " << BASE_BOOT_BLK << std::endl;
    std::cout << "    super on block " << BASE_SUPER_BLK << std::endl;
    std::cout << "    bitmap on block " << BASE_BITMAP_BLK << std::endl;
    std::cout << "    inode block start on block " << BASE_INODE_BLK
              << std::endl;
    std::cout << "    data block start on block " << BASE_DATA_BLKS
              << std::endl;
    std::cout << std::endl;
    std::cout << "Total inodes: " << N_INODES << std::endl;
    std::cout << "Total data blocks: " << N_DBLKS << std::endl;
}

int main(int argc, char *argv[]) {
    char *blk_root;
    if (argc < 2) {
        printf("Usage: %s [block_root]\n", argv[0]);
        return -1;
    } else {
        blk_root = argv[1];
    }

    // Create underlying files for virtual block device
    if (mkdir(blk_root, 0777) != 0) {
        perror("mkdir");
        return -1;
    }
    char fname[PATH_MAX + 1];
    char buf[aqfs::BLKSIZE];
    for (int i = 0; i < aqfs::NBLKS; i++) {
        sprintf(fname, "%s/blk_%04d", blk_root, i);
        std::ofstream f(fname);
        f.write(buf, aqfs::BLKSIZE);
    }

    print_paras();

    using namespace aqfs;

    // Init runtime
    Runtime::init(blk_root);

    // init super block
    Runtime::super.magic = 0xdeadbeef;

    // init bitmap block
    Runtime::bitmap = bitmap_t();

    // Reserve blocks
    for (int i = 0; i < aqfs::BASE_DATA_BLKS; i++)
        Runtime::bitmap.dmap.set(i);

    // Reserve inode 0 (null), 1 (root)
    Runtime::bitmap.imap.set(0);
    Runtime::bitmap.imap.set(1);

    /* init root directory */
    dir_t rootdir(1);
    rootdir.setmode(S_IFDIR | 0755);
    rootdir.add(1, ".");
    rootdir.add(1, "..");
    rootdir.addref();

    Runtime::fini();

    return 0;
}
