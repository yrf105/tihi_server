#ifndef TIHI_UTILS_ENDIAN_H
#define TIHI_UTILS_ENDIAN_H

#define TIHI_LITTLE_ENDIAN 1
#define TIHI_BIG_ENDIAN 2

#include <endian.h>
#include <byteswap.h>
#include <stdint.h>

#include <type_traits>

namespace tihi {
template <typename T>
typename std::enable_if<sizeof(T) == sizeof(uint64_t), T>::type
byteswap(T value) {
    return static_cast<T>(bswap_64(static_cast<uint64_t>(value)));
}

template <typename T>
typename std::enable_if<sizeof(T) == sizeof(uint32_t), T>::type
byteswap(T value) {
    return static_cast<T>(bswap_32(static_cast<uint32_t>(value)));
}

template <typename T>
typename std::enable_if<sizeof(T) == sizeof(uint16_t), T>::type
byteswap(T value) {
    return static_cast<T>(bswap_16(static_cast<uint16_t>(value)));
}

#if BYTE_ORDER == BIG_ENDIAN
#define TIHI_BYTE_ORDER TIHI_BIG_ENDIAN
#else
#define TIHI_BYTE_ORDER TIHI_LITTLE_ENDIAN
#endif

#if TIHI_BYTE_ORDER == TIHI_BIG_ENDIAN
template <typename T>
T byteswapOnLittleEndian(T value) {
    return value;
}

template <typename T>
T byteswapOnBigLittleEndian(T value) {
    return byteswap(value);
}

template <typename T>
T hton(T port) {
    return port;
}

template <typename T>
T ntoh(T value) {
    return value;
}

#else
template <typename T>
T byteswapOnLittleEndian(T value) {
    return byteswap(value);
}

template <typename T>
T byteswapOnBigLittleEndian(T value) {
    return value;
}

template <typename T>
T hton(T value) {
    return byteswap(value);
}

template <typename T>
T ntoh(T value) {
    return byteswap(value);
}

#endif

} // namespace tihi

#endif // TIHI_UTILS_ENDIAN_H