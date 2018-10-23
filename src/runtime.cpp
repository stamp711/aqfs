#include "runtime.h"

namespace aqfs::Runtime {

disk_t disk;
super_t super;
bitmap_t bitmap;

int init(std::string disk_root) {
    disk.setroot(disk_root);
    super.load();
    bitmap.load();
    super.clean = 0;
    super.persist();
    return 0;
}

int fini() {
    super.clean = 1;
    bitmap.persist();
    super.persist();
    return 0;
}

} // namespace aqfs::Runtime
