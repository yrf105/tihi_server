#include "fd_manager.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include "fcntl.h"

#include "hook.h"

namespace tihi {

FdCtx::FdCtx(int fd)
    : is_init_(false),
      is_socket_(false),
      sys_nonblock_(false),
      user_nonblock_(false),
      is_closed_(true),
      fd_(fd),
      recv_timeout_(-1),
      send_timeout_(-1) {
    init();
}

FdCtx::~FdCtx() {
    if (!is_closed()) {
        close();
    }
}

uint64_t FdCtx::timeout(int type) {
    if (SO_RCVTIMEO == type) {
        return recv_timeout_;
    } else if (SO_SNDTIMEO == type) {
        return send_timeout_;
    }
    return -1ul;
}

void FdCtx::set_timeout(int type, uint64_t v) {
    if (SO_RCVTIMEO == type) {
        recv_timeout_ = v;
    } else if (SO_SNDTIMEO == type) {
        recv_timeout_ = v;
    }
}

bool FdCtx::init() {
    if (is_init()) {
        return true;
    }

    recv_timeout_ = -1;
    send_timeout_ = -1;

    struct stat fd_state;
    if (-1 == fstat(fd_, &fd_state)) {
        is_init_ = false;
        is_socket_ = false;
    } else {
        is_init_ = true;
        is_socket_ = S_ISSOCK(fd_state.st_mode);
    }

    /**
     * 如果是 socket fd 则设置系统 非阻塞
    */
    if (is_socket()) {
        int flag = fcntl_f(fd_, F_GETFL, 0);
        if (!(flag & O_NONBLOCK)) {
            fcntl_f(fd_, F_SETFL, flag | O_NONBLOCK);
        }
        sys_nonblock_ = true;
    } else {
        sys_nonblock_ = false;
    }

    user_nonblock_ = false;
    is_closed_ = false;

    return is_init_;
}

void FdCtx::close() {
    ::close(fd_);
    is_closed_ = true;
}

FdManager::FdManager() {
    fds_.resize(32);
}

FdCtx::ptr FdManager::fd(int fd, bool auto_create) {
    if (fd < 0) {
        return nullptr;
    } 

    rwmutex_type::read_lock lock(mutex_);
    if (fds_.size() <= (size_t)fd) {
        if (auto_create) {
            lock.unlock();
            rwmutex_type::write_lock lock2(mutex_);
            fds_.resize(fd * 1.5);
            FdCtx::ptr tmp(new FdCtx(fd));
            fds_[fd] = tmp;
            return fds_[fd];
        }
    } else {
        if (fds_[fd]) {
            return fds_[fd];
        } else {
            if (auto_create) {
                lock.unlock();
                rwmutex_type::write_lock lock2(mutex_);
                FdCtx::ptr tmp(new FdCtx(fd));
                fds_[fd] = tmp;
                return fds_[fd];
            }
        }
    }

    return nullptr;
}

void FdManager::delFd(int fd) {
    rwmutex_type::write_lock lock(mutex_);
    if (fds_.size() <= (size_t)fd) {
        return ;
    }

    fds_[fd].reset();
}

}  // namespace tihi