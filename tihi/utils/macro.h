#ifndef TIHI_UTILS_MACRO_H_
#define TIHI_UTILS_MACRO_H_

#include <assert.h>

#include "log/log.h"
#include "utils.h"

#if defined __GNUC__ || defined __llvm__
#   define TIHI_LIKELY(x)       __builtin_expect(!!(x), 1)
#   define TIHI_UNLIKELY(x)     __builtin_expect(!!(x), 0)
#else
#   define TIHI_LIKELY(x)       (x)
#   define TIHI_UNLIKELY(x)     (x)
#endif

#define TIHI_ASSERT(expr)                                              \
    do {                                                               \
        if (TIHI_UNLIKELY(!(expr))) {                                                   \
            TIHI_LOG_FATAL(TIHI_LOG_ROOT())                            \
                << "\nbacktrace: " << tihi::Backtrace(0, 100, "    "); \
            assert(expr);                                              \
        }                                                              \
    } while (0)

#define TIHI_ASSERT2(expr, msg)                                                \
    do {                                                                       \
        if (TIHI_UNLIKELY(!(expr))) {                                                           \
            TIHI_LOG_FATAL(TIHI_LOG_ROOT())                                    \
                << msg << " \nbacktrace: " << tihi::Backtrace(0, 100, "    "); \
            assert(expr);                                                      \
        }                                                                      \
    } while (0)

#endif  // TIHI_UTILS_MACRO_H_