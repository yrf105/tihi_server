#ifndef TIHI_UTILS_H_
#define TIHI_UTILS_H_

#include <stdint.h>
#include <unistd.h>
#include <sys/time.h>

#include <vector>
#include <string>

namespace tihi {
pid_t ThreadId();
uint32_t FiberId();
void Backtrace(std::vector<std::string>& res, int offset, int size);
std::string Backtrace(int offset, int size, const std::string& prefix = "");
uint64_t MS();
uint64_t US();
}  // namespace tihi

#endif  // TIHI_UTILS_H_