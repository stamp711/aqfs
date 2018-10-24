#pragma once

#include <string>

namespace aqfs {

class disk_t {
    std::string root;

  public:
    void setroot(std::string root) { this->root = root; }
    int read(uint32_t blkno, char *buf);
    int write(uint32_t blkno, char *buf);
};

} // namespace aqfs
