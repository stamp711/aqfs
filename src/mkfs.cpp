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
    std::cout << "size of struct inode: " << sizeof(struct inode) << std::endl;
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

    // Reserve inode 0 (not used), 1 (root)
    Runtime::bitmap.imap.set(0);

    /* init root directory */
    dir_t rootdir(1);
    rootdir.setmode(S_IFDIR);
    rootdir.add(1, ".");
    rootdir.add(1, "..");
    rootdir.addref();

    Runtime::fini();

    return 0;
}
