#include "scheduler.h"

#include "log/log.h"
#include "utils/macro.h"

namespace tihi {

static Logger::ptr g_sys_logger = TIHI_LOG_LOGGER("system");

/**/
static thread_local Scheduler* t_scheduler = nullptr;
static thread_local Fiber* t_fiber = nullptr;
extern thread_local Fiber::ptr t_threadFiber;

Scheduler::Scheduler(size_t threads, bool use_caller, const std::string& name)
    : name_(name) {
    if (use_caller) {
        /*初始化主协程*/
        Fiber::This();
        /*当前线程参与调度，需要新创建的线程数 -1*/
        --threads;
        /*当前线程没有调度器*/
        TIHI_ASSERT((nullptr == t_scheduler));
        t_scheduler = this;
        /*
        创建 scheduler 的线程的主协程就是 scheduler 的主协程
        */
        root_fiber_ = t_threadFiber;
        // /*再创建一个真正做事的协程做主协程*/
        // root_fiber_.reset(new Fiber(std::bind(&Scheduler::run, this)));
        // t_fiber = root_fiber_.get();
        // TIHI_LOG_DEBUG(g_sys_logger) << root_fiber_;
        // // t_threadFiber = root_fiber_;

        Thread::SetName(name_);
        root_thread_id_ = ThreadId();
        thread_ids_.push_back(root_thread_id_);
    } else {
        root_thread_id_ = -1;
    }
    thread_count_ = threads;
}

Scheduler::~Scheduler() {
    TIHI_ASSERT(stopping_);
    if (This() == this) {
        t_scheduler = nullptr;
    }
}

Scheduler* Scheduler::This() { return t_scheduler; }

Fiber* Scheduler::MainFiber() { return t_fiber; }

void Scheduler::start() {
    mutex_type::mutex lock(mutex_);
    if (!stopping_) {
        return;
    }
    stopping_ = false;
    TIHI_ASSERT((threads_.empty()));
    threads_.resize(thread_count_);
    for (size_t i = 0; i < thread_count_; ++i) {
        threads_[i].reset(
            new Thread(std::bind(&Scheduler::run, this), name_ + "_" + std::to_string(i)));
        thread_ids_.push_back(threads_[i]->id());
    }
}

// void Scheduler::stop() {
//     TIHI_LOG_INFO(g_sys_logger) << "scheduler stopping...";

//     auto_stop_ = true;

//     /*
//     use_caller == true 且只有一个线程的情况
//     那就是当前线程要结束自己
//     */
//     if (root_fiber_ && thread_count_ == 0 &&
//         (root_fiber_->state() == Fiber::TERM ||
//          root_fiber_->state() == Fiber::INIT)) {
//         stopping_ = true;
//         /*
//         给线程机会去处理后事
//         */
//         if (stopping()) {
//             return;
//         }
//     }

//     /*
//     有多个线程的情况
//     */
//     bool exit_on_this_fiber = false;
//     if (root_thread_id_ != -1) {
//         /*
//         若 use_caller == true，则必须是主线程来结束调度器
//         */
//         TIHI_ASSERT((This() == this));
//     } else {
//         /*
//         若 use_caller ！= true，则必须是其他线程来结束调度器
//         */
//         TIHI_ASSERT((This() != this));
//     }

//     stopping_ = true;
//     /*
//     调度器通知自己管理的所有线程自己要停止了
//     */
//     for (size_t i = 0; i < thread_count_; ++i) {
//         tickle();
//     }
//     /*
//     调度器通知主线程自己要停止了，如果有主线程 thread_count_ 是不算主线程的，
//     所以要多 tickle 一下
//     */
//     if (root_fiber_) {
//         tickle();
//     }

//     if (stopping()) {
//         TIHI_LOG_INFO(g_sys_logger) << "scheduler has stopped";

//         std::vector<Thread::ptr> thrs;
//         {
//             mutex_type::mutex lock(mutex_);
//             thrs.swap(threads_);
//         }

//         for (auto t : thrs) {
//             t->join();
//         }

//         return;
//     } else {
//         stopping_ = true;
//         run();
//         stop();
//     }
// }

void Scheduler::run_and_stop() {
    run();

    if (root_fiber_ == Fiber::This()) {
        stopping_ = true;

        /*
        调度器通知自己管理的所有线程自己要停止了
        */
        for (size_t i = 0; i < thread_count_; ++i) {
            tickle();
        }

        stop();
    }
}

void Scheduler::stop() {
    TIHI_LOG_INFO(g_sys_logger) << "Scheduler is stopping...";
    std::vector<Thread::ptr> thrs; 
    mutex_type::mutex lock(mutex_);
    thrs.swap(threads_);
    for (auto t : thrs) {
        t->join();
    }
}

void Scheduler::run() {
    /**
     * 告诉当前线程，调度器是谁
     */
    set_this();
    /**
     * 如果当前线程是创建调度器的线程，那么在创建调度器时就已经指定了主协程
     * 如果当前线程不是创建线程调度器的协程，则为当前线程创建主协程
     */
    if (root_thread_id_ != ThreadId()) {
        t_fiber = Fiber::This().get();
    }

    TIHI_LOG_INFO(g_sys_logger) << "run";

    Fiber::ptr idle_fiber(new Fiber(std::bind(&Scheduler::idle, this)));
    Fiber::ptr cb_fiber = nullptr;

    FiberOrFunction fof;
    while (true) {
        fof.clear();
        bool tickle_other = false;
        {
            mutex_type::mutex lock(mutex_);
            auto it = fibers_.begin();
            for (; it != fibers_.end(); ++it) {
                if (it->specific_thread_id != -1 &&
                    it->specific_thread_id != ThreadId()) {
                    tickle_other = true;
                    continue;
                }

                TIHI_ASSERT((it->fiber || it->cb));
                if (it->fiber) {
                    TIHI_ASSERT((it->fiber->state() != Fiber::EXEC));
                }

                fof = *it;
                fibers_.erase(it);
                ++active_thread_count_;
                break;
            }
        }

        if (tickle_other) {
            tickle();

            /**
             * 这里不能直接走，对于线程来说虽然当前的任务（Fiber or Function）自己不能
             * 执行但是自己应该去执行 idle
            */
            // continue ;
            // break;
        }

        if (fof.fiber && fof.fiber->state() != Fiber::TERM &&
            fof.fiber->state() != Fiber::EXCEP) {
            // ++active_thread_count_;
            fof.fiber->swapIn();
            --active_thread_count_;

            if (fof.fiber->state() == Fiber::READY) {
                schedule(fof.fiber);
            } else if (fof.fiber->state() != Fiber::TERM &&
                       fof.fiber->state() != Fiber::EXCEP) {
                fof.fiber->set_state(Fiber::HOLD);
            }
            fof.clear();
        } else if (fof.cb) {
            if (cb_fiber) {
                cb_fiber->reset(fof.cb);
            } else {
                cb_fiber.reset(new Fiber(fof.cb));
            }
            fof.clear();

            // ++active_thread_count_;
            cb_fiber->swapIn();
            --active_thread_count_;

            if (cb_fiber->state() == Fiber::READY) {
                schedule(cb_fiber);
            } else if (cb_fiber->state() == Fiber::TERM ||
                       cb_fiber->state() == Fiber::EXCEP) {
                cb_fiber->reset(nullptr);
            } else {
                cb_fiber->set_state(Fiber::HOLD);
                cb_fiber.reset();
            }
        } else {
            if (idle_fiber->state() == Fiber::TERM) { 
                break;
            }

            ++idle_thread_count_;
            idle_fiber->swapIn();
            --idle_thread_count_;
            if (idle_fiber->state() != Fiber::TERM && idle_fiber->state() != Fiber::EXCEP) {
                idle_fiber->set_state(Fiber::HOLD);
            }
        }
    }
}

void Scheduler::tickle() {
    TIHI_LOG_INFO(g_sys_logger) << "tickle";
}

bool Scheduler::stopping() {
    mutex_type::mutex lock(mutex_);
    // return auto_stop_ && stopping_ && fibers_.empty() && active_thread_count_ == 0;
    // return stopping_ && fibers_.empty() && active_thread_count_ == 0;
    // return fibers_.empty() && active_thread_count_ == 0;
    return true;
}

void Scheduler::set_this() { t_scheduler = this; }

void Scheduler::idle() {
    // if (!stopping()) {
        TIHI_LOG_INFO(g_sys_logger) << "idle...";
        // Fiber::This()->YieldToHold();
    // }
    // TIHI_LOG_INFO(g_sys_logger) << "idle over";
}

};  // namespace tihi