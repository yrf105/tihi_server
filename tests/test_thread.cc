#include "log/log.h"
#include "thread/thread.h"
#include "utils/utils.h"
#include "utils/mutex.h"
#include "config/config.h"

tihi::Logger::ptr g_logger = TIHI_LOG_ROOT();

int count = 0;
tihi::RWMutex s_mutex;
// tihi::Mutex s_mutex;
// pthread_mutex_t s_mutex;

void func1() {
    TIHI_LOG_DEBUG(g_logger)
        << "name: " << tihi::Thread::Name() << " id: " << tihi::ThreadId()
        << " this.name: " << tihi::Thread::This()->name()
        << " this.id: " << tihi::Thread::This()->id();
    // sleep(20);
    for (int i = 0; i < 100000; ++i) {
        // pthread_mutex_lock(&s_mutex);
        // s_mutex.lock();
        // tihi::Mutex::mutex lock(s_mutex);
        tihi::RWMutex::write_lock lock(s_mutex);
        ++count;
        // pthread_mutex_unlock(&s_mutex);
        // s_mutex.unlock();
    }
}

void func2() {
    while(true) {
        TIHI_LOG_INFO(g_logger) << "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx";
    }
}

void func3() {
    while(true) {
        TIHI_LOG_INFO(g_logger) << "--------------------------------------------------------------------------------";
    }
}

void test() {
    // TIHI_LOG_INFO(g_logger) << "start";
    // pthread_mutex_init(&s_mutex, nullptr);

    YAML::Node root = YAML::LoadFile("../bin/config/test_mutex_log.yml");
    tihi::Config::LoadFromYAML(root);

    std::vector<tihi::Thread::ptr> ths;
    for (int i = 0; i < 2; ++i) {
        tihi::Thread::ptr th(new tihi::Thread(&func2, "name_" + std::to_string(i * 2)));
        tihi::Thread::ptr th2(new tihi::Thread(&func3, "name_" + std::to_string(i * 2 + 1)));
        ths.push_back(th);
        ths.push_back(th2);
    }

    for (auto& t : ths) {
        t->join();
    }

    // pthread_mutex_destroy(&s_mutex);
    // TIHI_LOG_INFO(g_logger) << "count: " << count;
    // TIHI_LOG_INFO(g_logger) << "end";
}

int main(int argc, char** argv) {
    test();

    return 0;
}