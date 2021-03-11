#ifndef TIHI_UTILS_H_
#define TIHI_UTILS_H_

#include <stdint.h>
#include <unistd.h>

namespace tihi {
pid_t ThreadId();
uint32_t FiberId();
}  // namespace tihi

#endif  // TIHI_UTILS_H_