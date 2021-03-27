#include "bytearray/bytearray.h"
#include "log/log.h"
#include "utils/macro.h"

static tihi::Logger::ptr g_logger = TIHI_LOG_ROOT();

void test() {
#define TEST(type, write_f, read_f, len, base_len)               \
    do {                                                         \
        std::vector<type> vec;                                   \
        for (size_t i = 0; i < len; ++i) {                       \
            vec.push_back(rand());                               \
        }                                                        \
        tihi::ByteArray::ptr ba(new tihi::ByteArray(base_len));  \
        for (auto& i : vec) {                                    \
            ba->write_f(i);                                      \
        }                                                        \
        ba->set_postion(0);                                      \
        for (size_t i = 0; i < len; ++i) {                       \
            type v = ba->read_f();                               \
            TIHI_ASSERT(v == vec[i]);                            \
        }                                                        \
        TIHI_ASSERT((ba->read_size() == 0));                     \
        TIHI_LOG_INFO(g_logger)                                  \
            << "type: " << #type << " write_f: " << #write_f     \
            << " read_f: " << #read_f << " size: " << ba->size() \
            << " success";                                       \
    } while (0)

    TEST(int8_t, writeFint8, readFint8, 100, 1);
    TEST(uint8_t, writeFuint8, readFuint8, 100, 1);
    TEST(int16_t, writeFint16, readFint16, 100, 1);
    TEST(uint16_t, writeFuint16, readFuint16, 100, 1);
    TEST(int32_t, writeFint32, readFint32, 100, 1);
    TEST(uint32_t, writeFuint32, readFuint32, 100, 1);
    TEST(int64_t, writeFint64, readFint64, 100, 1);
    TEST(uint64_t, writeFuint64, readFuint64, 100, 1);

    TEST(int32_t, writeInt32, readInt32, 100, 1);
    TEST(uint32_t, writeUint32, readUint32, 100, 1);
    TEST(int64_t, writeInt64, readInt64, 100, 1);
    TEST(uint64_t, writeUint64, readUint64, 100, 1);

#undef TEST

#define TEST(type, write_f, read_f, len, base_len)                   \
    do {                                                             \
        std::vector<type> vec;                                       \
        for (size_t i = 0; i < len; ++i) {                           \
            vec.push_back(rand());                                   \
        }                                                            \
        tihi::ByteArray::ptr ba(new tihi::ByteArray(base_len));      \
        for (auto& i : vec) {                                        \
            ba->write_f(i);                                          \
        }                                                            \
        ba->set_postion(0);                                          \
        for (size_t i = 0; i < len; ++i) {                           \
            type v = ba->read_f();                                   \
            TIHI_ASSERT(v == vec[i]);                                \
        }                                                            \
        TIHI_ASSERT((ba->read_size() == 0));                         \
        ba->set_postion(0);                                          \
        ba->writeToFile("/tmp/" #type "_" #write_f ".dat");          \
        TIHI_ASSERT((ba->postion() == 0));                           \
        tihi::ByteArray::ptr ba2(new tihi::ByteArray(base_len * 2)); \
        ba2->readFromFile("/tmp/" #type "_" #write_f ".dat");        \
        ba2->set_postion(0);                                         \
        TIHI_ASSERT((ba->toString() == ba2->toString()));            \
        TIHI_ASSERT((ba->postion() == 0));                           \
        TIHI_ASSERT((ba2->postion() == 0));                          \
        TIHI_LOG_INFO(g_logger)                                      \
            << "type: " << #type << " write_f: " << #write_f         \
            << " read_f: " << #read_f << " size: " << ba->size()     \
            << " success";                                           \
    } while (0)

    TEST(int8_t, writeFint8, readFint8, 100, 1);
    TEST(uint8_t, writeFuint8, readFuint8, 100, 1);
    TEST(int16_t, writeFint16, readFint16, 100, 1);
    TEST(uint16_t, writeFuint16, readFuint16, 100, 1);
    TEST(int32_t, writeFint32, readFint32, 100, 1);
    TEST(uint32_t, writeFuint32, readFuint32, 100, 1);
    TEST(int64_t, writeFint64, readFint64, 100, 1);
    TEST(uint64_t, writeFuint64, readFuint64, 100, 1);

    TEST(int32_t, writeInt32, readInt32, 100, 1);
    TEST(uint32_t, writeUint32, readUint32, 100, 1);
    TEST(int64_t, writeInt64, readInt64, 100, 1);
    TEST(uint64_t, writeUint64, readUint64, 100, 1);

#undef TEST
}

int main(int argc, char** argv) {
    test();
    return 0;
}