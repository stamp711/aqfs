#ifndef AQFS_RUNTIME_H
#define AQFS_RUNTIME_H

#include "base.h"
#include "disk.h"

namespace aqfs::Runtime {

extern disk_t disk;
extern super_t super;
extern bitmap_t bitmap;

int init(std::string disk_root);
int fini();

} // namespace aqfs::Runtime

#endif
