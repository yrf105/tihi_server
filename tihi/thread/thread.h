#ifndef TIHI_THREAD_THREAD_H_
#define TIHI_THREAD_THREAD_H_

#include <pthread.h>
#include <unistd.h>

#include <functional>
#include <memory>
#include <string>

namespace tihi {

class Thread {
public:
    using ptr = std::shared_ptr<Thread>;

    Thread(std::function<void()> cb, const std::string& name);
    ~Thread();

    pid_t id() const { return id_; }
    const std::string& name() const { return name_; }


    static Thread* This();
    static const std::string Name();
    static void SetName(const std::string& name);

    void join();

private:
    Thread(const Thread&) = delete;
    Thread(const Thread&&) = delete;
    Thread& operator=(const Thread&) = delete;

    static void* run(void*);

private:
    std::function<void()> cb_;
    std::string name_;
    pid_t id_ = -1;
    pthread_t thread_ = 0;
};

}  // namespace tihi

#endif  // THREAD_THREAD_H_