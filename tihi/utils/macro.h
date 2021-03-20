#ifndef TIHI_UTILS_MACRO_H_
#define TIHI_UTILS_MACRO_H_

#include <assert.h>

#include "log/log.h"
#include "utils.h"

#define TIHI_ASSERT(expr)                                              \
    do {                                                               \
        if (!(expr)) {                                                   \
            TIHI_LOG_FATAL(TIHI_LOG_ROOT())                            \
                << "\nbacktrace: " << tihi::Backtrace(0, 100, "    "); \
            assert(expr);                                              \
        }                                                              \
    } while (0)

#define TIHI_ASSERT2(expr, msg)                                                \
    do {                                                                       \
        if (!(expr)) {                                                           \
            TIHI_LOG_FATAL(TIHI_LOG_ROOT())                                    \
                << msg << " \nbacktrace: " << tihi::Backtrace(0, 100, "    "); \
            assert(expr);                                                      \
        }                                                                      \
    } while (0)

#endif  // TIHI_UTILS_MACRO_H_