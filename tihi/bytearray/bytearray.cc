#include "bytearray.h"

#include <cstring>
#include <sstream>
#include <fstream>
#include <iomanip>

#include "log/log.h"
#include "utils/endian.h"

namespace tihi {

static Logger::ptr g_sys_logger = TIHI_LOG_LOGGER("system");

ByteArray::Node::Node(size_t sz) : ptr(new char[sz]), next(nullptr), size(sz) {}

ByteArray::Node::Node() : ptr(nullptr), next(nullptr), size(0) {}

ByteArray::Node::~Node() {
    if (ptr) {
        delete[] ptr;
    }
}

ByteArray::ByteArray(size_t base_size)
    : base_size_(base_size),
      curr_pos_(0),
      capacity_(base_size),
      size_(0),
      endian_(TIHI_BIG_ENDIAN),
      root_(new Node(base_size)),
      curr_(root_) {}

ByteArray::~ByteArray() {
    Node* tmp = root_;
    while (tmp) {
        curr_ = tmp;
        tmp = tmp->next;
        delete curr_;
    }
}

void ByteArray::writeFint8(int8_t val) { write(&val, sizeof(val)); }

void ByteArray::writeFuint8(uint8_t val) { write(&val, sizeof(val)); }

void ByteArray::writeFint16(int16_t val) {
    if (endian_ != TIHI_BYTE_ORDER) {
        val = byteswap(val);
    }
    write(&val, sizeof(val));
}

void ByteArray::writeFuint16(uint16_t val) {
    if (endian_ != TIHI_BYTE_ORDER) {
        val = byteswap(val);
    }
    write(&val, sizeof(val));
}

void ByteArray::writeFint32(int32_t val) {
    if (endian_ != TIHI_BYTE_ORDER) {
        val = byteswap(val);
    }
    write(&val, sizeof(val));
}

void ByteArray::writeFuint32(uint32_t val) {
    if (endian_ != TIHI_BYTE_ORDER) {
        val = byteswap(val);
    }
    write(&val, sizeof(val));
}

void ByteArray::writeFint64(int64_t val) {
    if (endian_ != TIHI_BYTE_ORDER) {
        val = byteswap(val);
    }
    write(&val, sizeof(val));
}

void ByteArray::writeFuint64(uint64_t val) {
    if (endian_ != TIHI_BYTE_ORDER) {
        val = byteswap(val);
    }
    write(&val, sizeof(val));
}

static uint32_t EncodeZigzag32(int32_t val) {
    if (val < 0) {
        return ((uint32_t)(-val)) * 2 - 1;
    }
    return (uint32_t)(val * 2);
}

static int32_t DecodeZigzag32(uint32_t val) { return (val >> 1) ^ -(val & 1); }

void ByteArray::writeInt32(int32_t val) { writeUint32(EncodeZigzag32(val)); }

void ByteArray::writeUint32(uint32_t val) {
    uint8_t tmp[5];
    uint8_t i = 0;
    while (val >= 0x80) {
        tmp[i++] = (val & 0x7f) | 0x80;
        val >>= 7;
    }
    tmp[i++] = val;
    write(tmp, i);
}

static uint64_t EncodeZigzag64(int64_t val) {
    if (val < 0) {
        return ((uint64_t)(-val)) * 2 - 1;
    }
    return (uint64_t)(val * 2);
}

static int64_t DecodeZigzag64(uint64_t val) { return (val >> 1) ^ -(val & 1); }

void ByteArray::writeInt64(int64_t val) { writeUint64(EncodeZigzag64(val)); }

void ByteArray::writeUint64(uint64_t val) {
    uint8_t tmp[10];
    uint8_t i = 0;
    while (val >= 0x80) {
        tmp[i++] = (val & 0x7f) | 0x80;
        val >>= 7;
    }
    tmp[i++] = val;
    write(tmp, i);
}

void ByteArray::writeFloat(float val) {
    uint32_t v;
    memcpy(&v, &val, sizeof(v));
    writeUint32(v);
}

void ByteArray::writeDouble(double val) {
    uint64_t v;
    memcpy(&v, &val, sizeof(v));
    writeUint64(v);
}

void ByteArray::writeStringF16(const std::string& val) {
    writeFuint16(val.size());
    write(val.c_str(), val.size());
}

void ByteArray::writeStringF32(const std::string& val) {
    writeFuint32(val.size());
    write(val.c_str(), val.size());
}

void ByteArray::writeStringF64(const std::string& val) {
    writeFuint64(val.size());
    write(val.c_str(), val.size());
}

void ByteArray::writeStringVint(const std::string& val) {
    writeFuint64(val.size());
    write(val.c_str(), val.size());
}

void ByteArray::writeStringWithoutLength(const std::string& val) {
    write(val.c_str(), val.size());
}

int8_t ByteArray::readFint8() {
    int8_t v;
    read(&v, sizeof(v));
    return v;
}

uint8_t ByteArray::readFuint8() {
    uint8_t v;
    read(&v, sizeof(v));
    return v;
}

#define XX(type)                          \
    do {                                  \
        type v;                           \
        read(&v, sizeof(v));              \
        if (endian_ == TIHI_BYTE_ORDER) { \
            return v;                     \
        }                                 \
        return byteswap(v);               \
    } while (0)

int16_t ByteArray::readFint16() { XX(int16_t); }

uint16_t ByteArray::readFuint16() { XX(uint16_t); }

int32_t ByteArray::readFint32() { XX(int32_t); }

uint32_t ByteArray::readFuint32() { XX(uint32_t); }

int64_t ByteArray::readFint64() { XX(int64_t); }

uint64_t ByteArray::readFuint64() { XX(uint64_t); }

#undef XX

int32_t ByteArray::readInt32() { return DecodeZigzag32(readUint32()); }

uint32_t ByteArray::readUint32() {
    uint32_t res = 0;
    uint8_t v = 0;
    for (size_t i = 0; i < 32; i += 7) {
        read(&v, sizeof(v));
        if (v < 0x80) {
            res |= ((uint32_t)v << i);
            break;
        } else {
            res |= (((uint32_t)v & 0x7f) << i);
        }
    }

    return res;
}

int64_t ByteArray::readInt64() { return DecodeZigzag64(readUint64()); }

uint64_t ByteArray::readUint64() {
    uint64_t res = 0;
    uint8_t v = 0;
    for (size_t i = 0; i < 64; i += 7) {
        read(&v, sizeof(v));
        if (v < 0x80) {
            res |= ((uint64_t)v << i);
            break;
        } else {
            res |= (((uint64_t)v & 0x7f) << i);
        }
    }

    return res;
}

float ByteArray::readFloat() {
    uint32_t v = readFuint32();
    float val;
    memcpy(&val, &v, sizeof(v));
    return val;
}

double ByteArray::readDouble() {
    uint64_t v = readFuint32();
    double val;
    memcpy(&val, &v, sizeof(v));
    return val;
}

std::string ByteArray::readStringF16() {
    uint16_t len = readFuint16();
    std::string buff(len, '\0');
    read(&buff[0], len);
    return buff;
}

std::string ByteArray::readStringF32() {
    uint32_t len = readFuint32();
    std::string buff(len, '\0');
    read(&buff[0], len);
    return buff;
}

std::string ByteArray::readStringF64() {
    uint64_t len = readFuint64();
    std::string buff(len, '\0');
    read(&buff[0], len);
    return buff;
}

std::string ByteArray::readStringVint() {
    uint64_t len = readFuint64();
    std::string buff(len, '\0');
    read(&buff[0], len);
    return buff;
}

void ByteArray::clear() {
    curr_pos_ = size_ = 0;
    capacity_ = base_size_;
    Node* tmp = root_->next;
    while (tmp) {
        curr_ = tmp;
        tmp = tmp->next;
        delete curr_;
    }

    curr_ = root_;
    root_->next = nullptr;
}

void ByteArray::write(const void* buf, size_t sz) {
    if (sz == 0) {
        return;
    }

    addCapacity(sz + capacity_);

    size_t npos = curr_pos_ % base_size_;
    size_t ncap = curr_->size - npos;
    size_t buf_pos = 0;

    while (sz > 0) {
        if (ncap >= sz) {
            memcpy(curr_->ptr + npos, (const char*)buf + buf_pos, sz);
            if (curr_->size == (npos + sz)) {
                curr_ = curr_->next;
            }
            buf_pos += sz;
            curr_pos_ += sz;
            sz = 0;
        } else {
            memcpy(curr_->ptr + npos, (const char*)buf + buf_pos, ncap);
            curr_ = curr_->next;
            curr_pos_ += ncap;
            sz -= ncap;
            buf_pos += ncap;
            npos = 0;
            ncap = curr_->size;
        }
    }

    if (curr_pos_ > size_) {
        size_ = curr_pos_;
    }
}

void ByteArray::read(void* buf, size_t sz) {
    if (sz > read_size()) {
        throw std::out_of_range("not enough length");
    }

    size_t npos = curr_pos_ % base_size_;
    size_t ncap = curr_->size - npos;
    size_t buf_pos = 0;

    while (sz > 0) {
        if (ncap >= sz) {
            memcpy((char*)buf + buf_pos, curr_->ptr + npos, sz);
            if (curr_->size == (npos + sz)) {
                curr_ = curr_->next;
            }
            npos += sz;
            curr_pos_ += sz;
            buf_pos += sz;
            sz = 0;
        } else {
            memcpy((char*)buf + buf_pos, curr_->ptr + npos, ncap);
            curr_ = curr_->next;
            curr_pos_ += ncap;
            sz -= ncap;
            buf_pos += ncap;
            npos = 0;
            ncap = curr_->size;
        }
    }
}

void ByteArray::read(void* buf, size_t sz) const {
    if (sz > read_size()) {
        throw std::out_of_range("not enough length");
    }

    size_t curr_pos = postion();
    Node* cur = curr_;
    size_t npos = curr_pos % base_size_;
    size_t ncap = curr_->size - npos;
    size_t buf_pos = 0;

    while (sz > 0) {
        if (ncap >= sz) {
            memcpy((char*)buf + buf_pos, cur->ptr + npos, sz);
            if (cur->size == (npos + sz)) {
                cur = cur->next;
            }
            npos += sz;
            curr_pos += sz;
            buf_pos += sz;
            sz = 0;
        } else {
            memcpy((char*)buf + buf_pos, cur->ptr + npos, ncap);
            cur = cur ->next;
            curr_pos += ncap;
            sz -= ncap;
            buf_pos += ncap;
            npos = 0;
            ncap = cur->size;
        }
    }
}


void ByteArray::set_postion(size_t pos) {
    if (pos > capacity_) {
        throw std::out_of_range("set position out of range");
    }

    if (pos > size_) {
        size_ = pos;
    }

    curr_pos_ = pos;
    curr_ = root_;
    while (pos > curr_->size) {
        pos -= curr_->size;
        curr_ = curr_->next;
    }
    if (pos == curr_->size) {
        curr_ = curr_->next;
    }
}

bool ByteArray::writeToFile(const std::string& filename) const {
    std::ofstream ofs;
    ofs.open(filename, std::ios::trunc | std::ios::binary);
    if (!ofs) {
        TIHI_LOG_ERROR(g_sys_logger) << "文件打开失败 errno=" << errno
                                     << " strerror=" << strerror(errno);
        return false;
    }

    size_t read_sz = read_size();
    size_t npos = curr_pos_ % base_size_;
    Node* cur = curr_;
    size_t ncap = cur->size - npos;

    while (read_sz > 0) {
        if (ncap > read_sz) {
            ofs.write(cur->ptr + npos, read_sz);
            read_sz = 0;
        } else {
            ofs.write(cur->ptr + npos, ncap);
            read_sz -= ncap;
            cur = cur->next;
            ncap = cur->size;
            npos = 0;
        }
    }

    ofs.close();
    return true;
}

bool ByteArray::readFromFile(const std::string& filename) {
    std::ifstream ifs;
    ifs.open(filename, std::ios::binary);
    if (!ifs) {
        TIHI_LOG_ERROR(g_sys_logger) << "文件打开失败 errno=" << errno
                                     << " strerror=" << strerror(errno);
        return false;
    }

    std::shared_ptr<char> buffer(new char[base_size()], [](char* ptr){delete ptr;});
    while (!ifs.eof()) {
        ifs.read(buffer.get(), base_size());
        write(buffer.get(), ifs.gcount());
    }

    ifs.close();
    return true;
}

void ByteArray::addCapacity(size_t sz) {
    if (sz <= 0) {
        return ;
    }

    size_t old_cap = capacity();
    if (old_cap >= sz) {
        return ;
    }

    sz = sz - old_cap;
    size_t c = sz / base_size() + ((sz % base_size() > old_cap) ? 1 : 0);
    Node* tmp = root_;
    while (tmp->next) {
        tmp = tmp->next;
    }

    Node* head = nullptr;
    while (c > 0) {
        tmp->next = new Node(base_size_);
        if (!head) {
            head = tmp;
        }
        capacity_ += base_size_;
        --c;
        tmp = tmp->next;
    }

    if (old_cap == 0) {
        curr_ = head;
    }
}

bool ByteArray::isLittleEndian() const { return endian_ == TIHI_LITTLE_ENDIAN; }

void ByteArray::set_endian(bool flag) {
    if (flag) {
        endian_ = TIHI_LITTLE_ENDIAN;
    } else {
        endian_ = TIHI_BIG_ENDIAN;
    }
}

std::string ByteArray::toString() const {
    std::string str;
    str.resize(read_size());
    if (str.empty()) {
        return str;
    }
    
    read(&(str[0]), read_size());

    return str;
}

std::string ByteArray::toHexString() const {
    std::string str = toString();
    std::stringstream ss;

    for (size_t i = 0; i < str.size(); ++i) {
        if (i > 0 && i % 32 == 0) {
            ss << std::endl;
        }

        ss << std::setw(2) << std::setfill('0') << std::hex
           << (int)(int8_t)str[i] << " ";
    }

    return ss.str();
}

uint64_t ByteArray::getReadBuffers(std::vector<iovec>& buffers, uint64_t len) const {
    len = len > read_size() ? read_size() : len;
    if (len <= 0) {
        return 0;
    }

    uint64_t npos = curr_pos_ % base_size_;
    uint64_t ncap = curr_->size - npos;
    Node* cur = curr_;
    struct iovec iov;

    uint64_t size = len;
    while (len > 0) {
        if (ncap >= len) {
            iov.iov_base = cur->ptr + npos;
            iov.iov_len = len;
            len = 0;
        } else {
            iov.iov_base = cur->ptr + npos;
            iov.iov_len = ncap;
            len -= ncap;
            cur = cur->next;
            npos = 0;
            ncap = cur->size;
        }
        buffers.push_back(iov);
    }

    return size;
}

const uint64_t ByteArray::getReadBuffers(std::vector<iovec>& buffers, uint64_t len, uint64_t pos) const {
    len = len > read_size() ? read_size() : len;

    if (len + pos > size_) {
        throw std::out_of_range("getReadBuffers out-of-range");
    }

    if (len <= 0) {
        return 0;
    }

    uint64_t npos = pos % base_size_;
    uint32_t count = pos / base_size_;
    Node* cur = root_;
    while (count) {
        cur = cur->next;
        --count;
    }

    uint64_t ncap = cur->size - npos;
    struct iovec iov;

    uint64_t size = len;
    while (len > 0) {
        if (ncap >= len) {
            iov.iov_base = cur->ptr + npos;
            iov.iov_len = len;
            len = 0;
        } else {
            iov.iov_base = cur->ptr + npos;
            iov.iov_len = ncap;
            len -= ncap;
            cur = cur->next;
            npos = 0;
            ncap = cur->size;
        }
        buffers.push_back(iov);
    }

    return size;
}

uint64_t ByteArray::getWriteBuffers(std::vector<iovec>& buffers, uint64_t len) {
    if (len <= 0) {
        return 0;
    }

    addCapacity(len);

    uint64_t npos = curr_pos_ % base_size_;
    Node* cur = curr_;
    uint64_t ncap = cur->size - npos;
    struct iovec iov;

    uint64_t size = len;
    while (len > 0) {
        if (ncap >= len) {
            iov.iov_base = cur->ptr + npos;
            iov.iov_len = len;
            len = 0;
        } else {
            iov.iov_base = cur->ptr + npos;
            iov.iov_len = ncap;
            len -= ncap;
            cur = cur->next;
            npos = 0;
            ncap = cur->size;
        }
        buffers.push_back(iov);
    }

    return size;
}


}  // namespace tihi