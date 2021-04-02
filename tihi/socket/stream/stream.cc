#include "stream.h"

namespace tihi {

Stream::~Stream() {}

int Stream::readFixSize(void* buffer, size_t length) {
    int rt = 0;
    size_t offset = 0;
    size_t left = length;
    while (left > 0) {
        rt = read((char*)(buffer) + offset, left);
        if (rt <= 0) {
            return rt;
        }
        offset += rt;
        left -= rt;
    }
    return length;
}

int Stream::readFixSize(ByteArray::ptr ba, size_t length) {
    int rt = 0;
    size_t left = length;
    while (left > 0) {
        rt = read(ba, left);
        if (rt <= 0) {
            return rt;
        }
        left -= rt;
    }
    return length;
}

int Stream::writeFixSize(const void* buffer, size_t length) {
    int rt = 0;
    size_t offset = 0;
    size_t left = length;
    while (left > 0) {
        rt = write((const char*)(buffer) + offset, left);
        if (rt <= 0) {
            return rt;
        }
        offset += rt;
        left -= rt;
    }
    return length;
}

int Stream::writeFixSize(ByteArray::ptr ba, size_t length) {
    int rt = 0;
    size_t left = length;
    while (left > 0) {
        rt = write(ba, left);
        if (rt <= 0) {
            return rt;
        }
        left -= rt;
    }
    return length;
}

} // namespace tihi