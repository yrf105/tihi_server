#include "utils.h"

#include <sys/syscall.h>

namespace tihi {

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

} // namespace tihi