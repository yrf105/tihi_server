#ifndef TIHI_BYTE_ARRAY_BYTE_ARRAY_H_
#define TIHI_BYTE_ARRAY_BYTE_ARRAY_H_

#include <sys/types.h>
#include <sys/uio.h>

#include <memory>
#include <string>
#include <vector>

namespace tihi {

class ByteArray {
public:
    using ptr = std::shared_ptr<ByteArray>;

    struct Node {
        Node(size_t sz);
        Node();
        ~Node();

        char* ptr;
        Node* next;
        size_t size;
    };
    
    ByteArray(size_t base_size = 4096);
    ~ByteArray();

    // write
    // 固定长度
    void writeFint8(int8_t val);
    void writeFuint8(uint8_t val);
    void writeFint16(int16_t val);
    void writeFuint16(uint16_t val);
    void writeFint32(int32_t val);
    void writeFuint32(uint32_t val);
    void writeFint64(int64_t val);
    void writeFuint64(uint64_t val);

    void writeInt32(int32_t val);
    void writeUint32(uint32_t val);
    void writeInt64(int64_t val);
    void writeUint64(uint64_t val);

    void writeFloat(float val);
    void writeDouble(double val);
    // len:int16 data
    void writeStringF16(const std::string& val);
    // len:int32 data
    void writeStringF32(const std::string& val);
    // len:int64 data
    void writeStringF64(const std::string& val);
    // len:intvar data
    void writeStringVint(const std::string& val);
    // data
    void writeStringWithoutLength(const std::string& val);

    //read
    int8_t readFint8();
    uint8_t readFuint8();
    int16_t readFint16();
    uint16_t readFuint16();
    int32_t readFint32();
    uint32_t readFuint32();
    int64_t readFint64();
    uint64_t readFuint64();

    int32_t readInt32();
    uint32_t readUint32();
    int64_t readInt64();
    uint64_t readUint64();

    float readFloat();
    double readDouble();

    std::string readStringF16();
    std::string readStringF32();
    std::string readStringF64();
    std::string readStringVint();

    void clear();
    void write(const void* buf, size_t sz);
    void read(void* buf, size_t sz);
    void read(void* buf, size_t sz) const;

    size_t postion() const { return curr_pos_; };
    void set_postion(size_t pos);

    bool writeToFile(const std::string& filename) const;
    bool readFromFile(const std::string& filename);

    size_t base_size() const { return base_size_; }
    size_t read_size() const { return size_ - curr_pos_; }

    bool isLittleEndian() const;
    /**
     * true 为小端
    */
    void set_endian(bool flag);

    std::string toString() const;
    std::string toHexString() const;

    uint64_t getReadBuffers(std::vector<iovec>& buffers, uint64_t len = ~0ull) const;
    const uint64_t getReadBuffers(std::vector<iovec>& buffers, uint64_t len = ~0ull, uint64_t pos = 0) const;
    uint64_t getWriteBuffers(std::vector<iovec>& buffers, uint64_t len = ~0ull);

    size_t size() const { return size_; }

private:
    void addCapacity(size_t sz);
    size_t capacity() const { return capacity_; }

private:
    size_t base_size_;
    size_t curr_pos_;
    size_t capacity_;
    size_t size_;
    uint8_t endian_;

    Node* root_;
    Node* curr_;
};

} // namespace tihi

#endif // TIHI_BYTE_ARRAY_BYTE_ARRAY_H_