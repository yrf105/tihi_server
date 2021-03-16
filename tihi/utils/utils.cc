#include "utils.h"

#include <sys/syscall.h>
#include <execinfo.h>

#include "log/log.h"

namespace tihi {

static Logger::ptr g_sys_logger = TIHI_LOG_LOGGER("system");

pid_t ThreadId() {
    static __thread pid_t id = 0;
    if (id != 0) {
        return id;
    }
    id = syscall(SYS_gettid);
    return id;
}

uint32_t FiberId() {
    return 0;
}

void Backtrace(std::vector<std::string>& res, int offset, int size) {
    int total_size = offset + size;

    void** buffer = (void**)(malloc(sizeof(void*) * total_size));
    int nptrs = ::backtrace(buffer, total_size);
    char** strings = ::backtrace_symbols(buffer, nptrs);
    if (strings == NULL) {
        TIHI_LOG_ERROR(g_sys_logger) << "backtrace_symbols";
        return ;
    }

    for (int j = offset + 1; j < nptrs; ++j) {
        res.push_back(strings[j]);
    }

    free(strings);
    free(buffer);
}

std::string Backtrace(int offset, int size, const std::string& prefix) {
    std::vector<std::string> res;
    Backtrace(res, offset + 1, size);
    std::stringstream ss;
    for (const auto& s : res) {
        ss << std::endl << prefix << s;
    }

    return ss.str();
}

} // namespace tihi