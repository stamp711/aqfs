#include "base.h"
#include "disk.h"
#include "runtime.h"
#include <cstring>

namespace aqfs {

template <typename T> int blkbuf<T>::fill() {
    char buf[BLKSIZE];
    int res = Runtime::disk.read(this->blkno, buf);
    if (res != 0)
        return -1;
    std::memcpy(&this->data, buf, sizeof(data));
    return 0;
}

template <typename T> int blkbuf<T>::persist() {
    char buf[BLKSIZE];
    std::memcpy(buf, &this->data, sizeof(data));
    int res = Runtime::disk.write(this->blkno, buf);
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
    char buf[BLKSIZE];
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
    char buf[BLKSIZE];
    std::memcpy(buf, this, sizeof(bitmap_t));
    int res = Runtime::disk.write(BASE_BITMAP_BLK, buf);
    if (res != 0)
        return res;
    return 0;
}

} // namespace aqfs
