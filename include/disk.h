#ifndef AQFS_DISK_H
#define AQFS_DISK_H

#include <string>

namespace aqfs {

class disk_t {
    std::string root;

  public:
    void setroot(std::string root) { this->root = root; }
    int read(uint32_t blkino);
    int write(uint32_t blkino);
};

} // namespace aqfs

#endif