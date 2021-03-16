#include "fiber.h"

#include <atomic>

#include "config/config.h"
#include "utils/macro.h"

namespace tihi {

static Logger::ptr g_sys_logger = TIHI_LOG_LOGGER("system");

static std::atomic<uint64_t> s_fiber_id{0};
static std::atomic<uint64_t> s_total_fiber_counts{0};

// 线程中正在运行的协程
static thread_local Fiber* t_fiber = nullptr;

// 线程的主协程
static thread_local Fiber::ptr t_threadFiber = nullptr;

static ConfigVar<uint32_t>::ptr g_fiber_stack_size = Config::Lookup<uint32_t>(
    "fiber.stack_size", 1024ul * 1024ul, "fiber stack size");

class MallocAllocator {
public:
    static void* Alloc(size_t size) { return malloc(size); }

    static void Dealloc(void* ptr, size_t size) { free(ptr); }
};

using StackAllocator = MallocAllocator;

Fiber::Fiber() {
    state_ = EXEC;
    SetThis(this);

    TIHI_ASSERT2((0 == getcontext(&context_)), "getcontext");

    ++s_fiber_id;
    id_ = s_fiber_id;
    ++s_total_fiber_counts;
}

Fiber::Fiber(std::function<void()> cb, size_t stack_size)
    : id_(++s_fiber_id), cb_(cb) {
    ++s_total_fiber_counts;

    TIHI_ASSERT2((0 == getcontext(&context_)), "getcontext");

    stack_size_ = stack_size ? stack_size : g_fiber_stack_size->value();
    stack_ = StackAllocator::Alloc(stack_size_);
    context_.uc_stack.ss_sp = stack_;
    context_.uc_stack.ss_size = stack_size_;
    context_.uc_link = nullptr;

    makecontext(&context_, Fiber::MainFunc, 0);
}

Fiber::~Fiber() {
    if (stack_) {
        TIHI_ASSERT((state_ == INIT || state_ == TERM));
        StackAllocator::Dealloc(stack_, stack_size_);
        /*
        子协程的析构在主协程中完成
        */
        TIHI_LOG_DEBUG(g_sys_logger) << "sub-fiber destruct";
    } else {
        // 主协程退出了
        TIHI_ASSERT((!cb_));
        TIHI_ASSERT((state_ == EXEC));
        TIHI_ASSERT((stack_ == nullptr));
        TIHI_ASSERT((stack_size_ == 0));
        state_ = TERM;
        t_threadFiber = nullptr;

        Fiber* cur = t_fiber;
        if (cur == this) {
            SetThis(nullptr);
        }
        /*
        主协程的析构在没有任何协程执行时（所有子协程结束执行，主协程也结束执行）完成
        */
        TIHI_LOG_DEBUG(g_sys_logger) << "master-fiber destruct";
    }
    --s_total_fiber_counts;
}

void Fiber::reset(std::function<void()> cb) {
    TIHI_ASSERT((state_ == INIT || state_ == TERM));

    TIHI_ASSERT2((0 == getcontext(&context_)), "getcontext");

    context_.uc_stack.ss_sp = stack_;
    context_.uc_stack.ss_size = stack_size_;
    context_.uc_link = nullptr;

    cb_ = cb;
    makecontext(&context_, Fiber::MainFunc, 0);
}

void Fiber::swapIn() {
    TIHI_ASSERT((state_ != EXEC));
    state_ = EXEC;

    SetThis(this);

    TIHI_ASSERT2((0 == swapcontext(&(t_threadFiber->context_), &context_)),
                 "swapcontext");
}

void Fiber::swapOut() {
    /*就是 swapIn 的反向操作，具体状态切换交给调用者设置*/
    SetThis(t_threadFiber.get());
    TIHI_ASSERT2((0 == swapcontext(&context_, &(t_threadFiber->context_))),
                 "swapcontext");
}

void Fiber::SetThis(Fiber* fiber) { t_fiber = fiber; }

Fiber::ptr Fiber::This() {
    if (t_fiber) {
        return t_fiber->shared_from_this();
    }

    /*
    当前没有正在执行的协程，那么说明主协程还未创建（如果主协程创建了，那么至少有主协程在执行）,
    接下来就需要创建主协程
    */
    Fiber::ptr main_fiber(new Fiber);
    TIHI_ASSERT((t_fiber == main_fiber.get()));
    t_threadFiber = main_fiber;
    return t_fiber->shared_from_this();
}

void Fiber::YieldToReady() {
    Fiber::ptr curr = This();
    curr->state_ = READY;
    curr->swapOut();
}

void Fiber::YieldToHold() {
    Fiber::ptr curr = This();
    curr->state_ = HOLD;
    curr->swapOut();
}

uint64_t Fiber::TotalFiberCounts() { return s_total_fiber_counts; }

uint64_t Fiber::FiberId() {
    if (t_fiber) {
        return t_fiber->id();
    }

    return 0;
}

void Fiber::MainFunc() {
    Fiber::ptr curr = This();
    try {
        curr->cb_();
        curr->cb_ = nullptr;
        curr->state_ = TERM;
    } catch (std::exception& ex) {
        curr->state_ = EXCEP;
        TIHI_LOG_FATAL(g_sys_logger) << "Fiber Exception " << ex.what();
    } catch (...) {
        curr->state_ = EXCEP;
        TIHI_LOG_FATAL(g_sys_logger) << "Fiber Exception";
    }
    Fiber* raw_ptr = curr.get();
    curr.reset();
    raw_ptr->swapOut();

    /*
    子协程结束运行并返回到主协程执行析构，就不会再回到这里了
    */
    TIHI_ASSERT2(false, "sub-fiber: never reach!!!");
}

};  // namespace tihi