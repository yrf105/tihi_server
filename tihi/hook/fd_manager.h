#ifndef TIHI_HOOK_FD_MANAGER_H_
#define TIHI_HOOK_FD_MANAGER_H_

#include <memory>
#include <vector>

#include "utils/mutex.h"
#include "iomanager/iomanager.h"
#include "utils/singleton.h"

namespace tihi {

class FdCtx : public std::enable_shared_from_this<FdCtx> {
public:
    using ptr = std::shared_ptr<FdCtx>;
    FdCtx(int fd);
    ~FdCtx();

    bool init();
    bool is_init() const { return is_init_; }
    bool is_socket() const { return is_socket_; }
    void set_sys_nonblock(bool v) { sys_nonblock_ = v; }
    bool sys_nonblock() const { return sys_nonblock_; }
    void set_user_nonblock(bool v) { user_nonblock_ = v; }
    bool user_nonblock() const { return user_nonblock_; }
    void close();
    bool is_closed() const { return is_closed_; }

    uint64_t timeout(int type);
    void set_timeout(int type, uint64_t v);

private:
    bool is_init_: 1;
    bool is_socket_: 1;
    bool sys_nonblock_: 1;
    bool user_nonblock_: 1;
    bool is_closed_: 1;

    int fd_;

    uint64_t recv_timeout_;
    uint64_t send_timeout_;
};

class FdManager {
public:
    using rwmutex_type = RWMutex;    
    FdManager();

    FdCtx::ptr fd(int fd, bool auto_create = false);
    void delFd(int fd);

private:
    std::vector<FdCtx::ptr> fds_;

    rwmutex_type mutex_;
};

using FdMgr = SingletonPtr<FdManager>;

} // namespace tihi

#endif // TIHI_HOOK_FD_MANAGER_H_