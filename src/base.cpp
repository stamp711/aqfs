#include "base.h"
#include "disk.h"
#include "runtime.h"
#include <cstring>

namespace aqfs {

int blkbuf_t::fill() {
    int res = Runtime::disk.read(this->blkno, this->data);
    if (res != 0)
        return -1;
    return 0;
}

int blkbuf_t::persist() {
    int res = Runtime::disk.write(this->blkno, this->data);
    if (res != 0)
        return -1;
    return 0;
}

int super_t::load() {
    char buf[BLKSIZE];
    int res = Runtime::disk.read(BASE_SUPER_BLK, buf);
    if (res != 0)
        return -1;
    std::memcpy(this, buf, sizeof(super_t));
    return 0;
}

int super_t::persist() {
    char buf[BLKSIZE] = {0};
    std::memcpy(buf, this, sizeof(super_t));
    int res = Runtime::disk.write(BASE_SUPER_BLK, buf);
    if (res != 0)
        return res;
    return 0;
}

int bitmap_t::load() {
    char buf[BLKSIZE];
    int res = Runtime::disk.read(BASE_BITMAP_BLK, buf);
    if (res != 0)
        return -1;
    std::memcpy(this, buf, sizeof(bitmap_t));
    return 0;
}

int bitmap_t::persist() {
    char buf[BLKSIZE] = {0};
    std::memcpy(buf, this, sizeof(bitmap_t));
    int res = Runtime::disk.write(BASE_BITMAP_BLK, buf);
    if (res != 0)
        return res;
    return 0;
}

} // namespace aqfs
