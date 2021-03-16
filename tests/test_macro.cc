#include "utils/macro.h"
#include "utils/utils.h"
#include "log/log.h"

static tihi::Logger::ptr g_sys_logger = TIHI_LOG_LOGGER("system");

void test() {
    TIHI_LOG_DEBUG(g_sys_logger) << tihi::Backtrace(0, 10, "    ");

    TIHI_ASSERT(false);

    // TIHI_ASSERT2(false, "hello assert");
}

int main(int argc, char** argv) {

    test();

    return 0;
}