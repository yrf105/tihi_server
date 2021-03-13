#include "thread.h"

#include "log/log.h"
#include "utils/utils.h"

namespace tihi {

static thread_local Thread* t_thread = nullptr;
static thread_local std::string t_thread_name = "UNKNOW";

static tihi::Logger::ptr g_logger = TIHI_LOG_LOGGER("system");

Thread::Thread(std::function<void()> cb, const std::string& name)
    : cb_(cb), name_(name) {
    if (name.empty()) {
        name_ = "UNKNOW";
    }
    int rt = pthread_create(&thread_, nullptr, &Thread::run, this);
    if (rt) {
        TIHI_LOG_FATAL(g_logger)
            << "Thread create failed. rt: " << rt << " name: " << name;
        throw std::logic_error("pthread_create error");
    }

    // 信号量的值为零会阻塞，直到信号量的值大于零或者有信号中断调用
    semaphore_.wait();
}

Thread::~Thread() {
    if (thread_) {
        pthread_detach(thread_);
    }
}

Thread* Thread::This() { return t_thread; }

const std::string Thread::Name() { return t_thread_name; }

void Thread::SetName(const std::string& name) {
    if (t_thread) {
        t_thread->name_ = name;
    }
    t_thread_name = name;
}

void Thread::join() {
    if (thread_) {
        int rt = pthread_join(thread_, nullptr);
        if (rt) {
            TIHI_LOG_FATAL(g_logger)
                << "Thread join failed. rt: " << rt << " name: " << name_;
            throw std::logic_error("pthread_join error");
        }
        thread_ = 0;
    }
}

void* Thread::run(void* arg) {
    Thread* thread = (Thread*)(arg);
    t_thread = thread;
    thread->id_ = ThreadId();
    pthread_setname_np(pthread_self(), thread->name_.substr(0, 16).c_str());


    std::function<void()> cb;
    cb.swap(thread->cb_);

    thread->semaphore_.notify();

    cb();

    return 0;
}

}  // namespace tihi