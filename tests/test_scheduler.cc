#include "scheduler/scheduler.h"
#include "log/log.h"

static tihi::Logger::ptr t_logger = TIHI_LOG_ROOT();

void f() {
    TIHI_LOG_DEBUG(t_logger) << "hello scheduler";

    static int count = 5;
    sleep(1);

    if (--count >= 0) {
        tihi::Scheduler::This()->schedule(f, tihi::ThreadId());
    }
}

void f2() {
    // sleep(2);

    // static int count2 = 5;
    // sleep(1);
    // if (--count2 >= 0) {
        TIHI_LOG_DEBUG(t_logger) << "hello fibers";
    //     tihi::Scheduler::This()->schedule(f);
    // }

}

int main(int argc, char** argv) {

    TIHI_LOG_DEBUG(t_logger) << "main start";

    tihi::Scheduler sc(3, true);
    // sc.schedule(f2);
    sc.start();
    sc.schedule(f);
    sc.stop();

    TIHI_LOG_DEBUG(t_logger) << "main end";

    return 0;
} 