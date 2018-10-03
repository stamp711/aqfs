#include "disk.h"
#include "runtime.h"
#include <fstream>

namespace aqfs {

/* 需要 root 已经被设置 */
int disk_t::read(uint32_t blkno, char *buf) {
    std::string path = this->root + "/blk_" + std::to_string(blkno);
    std::ifstream f(path);

    if (f.read(buf, BLKSIZE))
        return 0;
    else
        return -1;
}

int disk_t::write(uint32_t blkno, char *buf) {
    std::string path = this->root + "/blk_" + std::to_string(blkno);
    std::ofstream f(path);

    if (f.write(buf, BLKSIZE))
        return 0;
    else
        return -1;
}

} // namespace aqfs
