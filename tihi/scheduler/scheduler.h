#ifndef TIHI_SCHEDULER_SCHEDULER_H_
#define TIHI_SCHEDULER_SCHEDULER_H_

#include <list>
#include <memory>
#include <string>
#include <vector>

#include "fiber/fiber.h"
#include "thread/thread.h"
#include "utils/mutex.h"

namespace tihi {

class Scheduler {
public:
    using ptr = std::shared_ptr<Scheduler>;
    using mutex_type = Mutex;

    Scheduler(size_t threads = 1, bool use_caller = true,
              const std::string& name = "");
    virtual ~Scheduler();

    const std::string& name() const { return name_; }

    static Scheduler* This();
    static Fiber* MainFiber();

    void start();
    void stop();
    void run_and_stop();

    template <typename F>
    void schedule(F fc, pid_t thread_id = -1) {
        bool need_tickle = false;
        {
            mutex_type::mutex lock(mutex_);
            need_tickle = scheduleNoLock(fc, thread_id);
        }

        if (need_tickle) {
            tickle();
        }
    }

    template <typename InputIterator>
    void schedule(InputIterator begin, InputIterator end) {
        bool need_tickle = false;
        {
            mutex_type::mutex lock(mutex_);
            while (begin != end) {
                need_tickle = scheduleNoLock(&*begin, -1) || need_tickle;
                ++begin;
            }
        }

        if (need_tickle) {
            tickle();
        }
    }

protected:
    virtual void tickle();
    void run();
    virtual bool stopping();
    virtual void idle();
    void set_this();

    bool hasIdleThread() { return idle_thread_count_ > 0; }
private:
    template <typename F>
    bool scheduleNoLock(F fc, pid_t thread_id) {
        bool need_tickle = fibers_.empty();
        FiberOrFunction ff(fc, thread_id);
        if (ff.fiber || ff.cb) {
            fibers_.push_back(ff);
        }

        return need_tickle;
    }


private:
    struct FiberOrFunction {
        Fiber::ptr fiber;
        std::function<void()> cb;
        pid_t specific_thread_id;  // fiber 或 cb 要在线程id 为 specific_thread_id 的线程上执行

        FiberOrFunction() : specific_thread_id(-1) {}
        FiberOrFunction(Fiber::ptr f, pid_t tid) : fiber(f), specific_thread_id(tid) {}
        FiberOrFunction(Fiber::ptr* f, pid_t tid) : specific_thread_id(tid) {
            fiber.swap(*f);
        }
        FiberOrFunction(std::function<void()> f, pid_t tid)
            : cb(f), specific_thread_id(tid) {}
        FiberOrFunction(std::function<void()>* f, pid_t tid) : specific_thread_id(tid) {
            cb.swap(*f);
        }

        void clear() {
            fiber = nullptr;
            cb = nullptr;
            specific_thread_id = -1;
        }
    };

private:
    std::vector<Thread::ptr> threads_;
    std::list<FiberOrFunction> fibers_;
    Fiber::ptr root_fiber_;
    std::string name_;

    mutex_type mutex_;

protected:
    std::vector<pid_t> thread_ids_;
    size_t thread_count_ = 0;
    std::atomic<size_t> active_thread_count_{0};
    std::atomic<size_t> idle_thread_count_{0};
    bool stopping_ = true;
    bool auto_stop_ = false;
    /**
     * 当创建调度器的线程也要被调度器调度时，
     * 使用此变量记录是谁创建了自己
    */
    pid_t root_thread_id_ = -1;
};

};  // namespace tihi

#endif  // TIHI_SCHEDULER_SCHEDULER_H_