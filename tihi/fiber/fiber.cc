#include "fiber.h"

#include <atomic>

#include "config/config.h"
#include "utils/macro.h"

namespace tihi {

static std::atomic<uint64_t> s_fiber_id{0};
static std::atomic<uint64_t> s_total_fiber_counts{0};

// 线程中正在运行的协程
static thread_local Fiber* t_fiber = nullptr;

// 线程的主协程
static thread_local std::shared_ptr<Fiber::ptr> t_threadFiber = nullptr;

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

    TIHI_ASSERT2(0 == getcontext(&context_), "getcontext");

    ++s_total_fiber_counts;
}

Fiber::Fiber(std::function<void()> cb, size_t stack_size = 0)
    : id_(++s_fiber_id), cb_(cb) {
    ++s_total_fiber_counts;

    TIHI_ASSERT2(0 == getcontext(&context_), "getcontext");

    stack_size_ = stack_size ? stack_size : g_fiber_stack_size->value();
    stack_ = StackAllocator::Alloc(stack_size_);
    context_.uc_stack.ss_sp = stack_;
    context_.uc_stack.ss_size = stack_size_;
    context_.uc_link = nullptr;

    makecontext(&context_, Fiber::MainFunc, 0);
}

Fiber::~Fiber() {
    --s_total_fiber_counts;
    if (stack_) {
        TIHI_ASSERT((state_ == INIT || state_ == TERM));
        StackAllocator::Dealloc(stack_, stack_size_);
    } else {
        // 主协程退出了
        TIHI_ASSERT((!cb_));
        TIHI_ASSERT((state_ == EXEC));
        // TIHI_ASSERT((stack_ == nullptr));
        // TIHI_ASSERT((stack_size_ == 0));

        Fiber* cur = t_fiber;
        if (cur == this) {
            SetThis(nullptr);
        }
    }
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

    TIHI_ASSERT2((0 == swapcontext(&((*t_threadFiber)->context_), &context_)),
                 "swapcontext");   
}

void Fiber::swapOut() {
    
}

Fiber::ptr Fiber::SetThis(Fiber* fiber) {}

Fiber::ptr This() {}

void Fiber::YieldToReady() {}

void Fiber::YieldToHold() {}

uint64_t Fiber::TotalFiberCounts() {}

void Fiber::MainFunc() {}

};  // namespace tihi