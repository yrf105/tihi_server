#include "log/log.h"
#include "thread/thread.h"
#include "utils/utils.h"

tihi::Logger::ptr g_logger = TIHI_LOG_ROOT();

void func1() {
    TIHI_LOG_DEBUG(g_logger)
        << "name: " << tihi::Thread::Name() << " id: " << tihi::ThreadId()
        << " this.name: " << tihi::Thread::This()->name()
        << " this.id: " << tihi::Thread::This()->id();
    sleep(20);
}

void func2() {}

void test() {
    TIHI_LOG_INFO(g_logger) << "start";

    std::vector<tihi::Thread::ptr> ths;
    for (int i = 0; i < 4; ++i) {
        tihi::Thread::ptr th(new tihi::Thread(&func1, "name_" + std::to_string(i + 1)));
        ths.push_back(th);
    }

    for (auto& t : ths) {
        t->join();
    }

    TIHI_LOG_INFO(g_logger) << "end";
}

int main(int argc, char** argv) {
    test();

    return 0;
}