#include "hook.h"

#include <dlfcn.h>
#include <errno.h>

#include "fd_manager.h"
#include "fiber/fiber.h"
#include "iomanager/iomanager.h"
#include "log/log.h"
#include "config/config.h"

namespace tihi {

static Logger::ptr g_sys_logger = TIHI_LOG_LOGGER("system");

static ConfigVar<int>::ptr g_tcp_connect_timeout = 
    Config::Lookup<int>("tcp.connect.timeout", 5000, "tcp connect timeout");

static thread_local bool t_hook_enable = false;

#define HOOK_FUN(XX) \
    XX(sleep)        \
    XX(usleep)       \
    XX(nanosleep)    \
    XX(socket)       \
    XX(connect)      \
    XX(accept)       \
    XX(read)         \
    XX(readv)        \
    XX(recv)         \
    XX(recvfrom)     \
    XX(recvmsg)      \
    XX(write)        \
    XX(writev)       \
    XX(send)         \
    XX(sendto)       \
    XX(sendmsg)      \
    XX(close)        \
    XX(fcntl)        \
    XX(ioctl)        \
    XX(getsockopt)   \
    XX(setsockopt)

void hook_init() {
    static bool is_inited = false;
    if (is_inited) {
        return;
    }

#define XX(name) name##_f = (name##_fun)dlsym(RTLD_NEXT, #name);
    HOOK_FUN(XX);
#undef XX
}

static int s_tcp_connect_timeout = -1;
struct __HookIniter {
    __HookIniter() {
        hook_init();
        s_tcp_connect_timeout = g_tcp_connect_timeout->value();

        g_tcp_connect_timeout->addListener([](const int old_value, const int new_value){
            s_tcp_connect_timeout = new_value;
        });
    }
};

static __HookIniter s_hook_initer;

bool is_hook_enable() { return t_hook_enable; }

void set_hook_enable(bool flag) { t_hook_enable = flag; }

struct timer_info {
    int cancelled = 0;
};

template <typename OriginFun, typename... Args>
ssize_t do_io(int fd, OriginFun fun, const char *hook_fun_name, uint32_t type,
              int timeout_type, Args &&...args) {
    if (!is_hook_enable()) {
        return fun(fd, std::forward<Args>(args)...);
    }

    FdCtx::ptr fdctx = FdMgr::GetInstance()->fd(fd);
    if (!fdctx) {
        return fun(fd, std::forward<Args>(args)...);
    }

    if (!fdctx->is_socket() || fdctx->user_nonblock()) {
        return fun(fd, std::forward<Args>(args)...);
    }

    if (fdctx->is_closed()) {
        errno = EBADFD;
        return -1;
    }

    uint64_t timeout = fdctx->timeout(timeout_type);
    std::shared_ptr<timer_info> tinfo(new timer_info);

retry:
    ssize_t n = fun(fd, std::forward<Args>(args)...);
    while (-1 == n && EINTR == errno) {
        n = fun(fd, std::forward<Args>(args)...);
    }
    if (-1 == n && EAGAIN == errno) {
        tihi::IOManager *iom = tihi::IOManager::This();
        tihi::Timer::ptr timer;
        std::weak_ptr<timer_info> wtinfo(tinfo);

        if ((uint64_t)-1 != timeout) {
            timer = iom->addConditionTimer(
                timeout,
                [wtinfo, fd, iom, type]() {
                    auto t = wtinfo.lock();
                    if (!t || t->cancelled) {
                        return;
                    }
                    t->cancelled = ETIMEDOUT;
                    iom->cancelEvent(fd, (tihi::IOManager::EventType)(type));
                },
                wtinfo);
        }

        /**
         * 回调函数为空则以当前协程为调度对象
         */
        int rt = iom->addEvent(fd, (tihi::IOManager::EventType)(type));
        if (rt) {
            TIHI_LOG_ERROR(g_sys_logger)
                << hook_fun_name << "addEvent(fd=" << fd << ", type=" << type
                << ")";
            if (timer) {
                timer->cancel();
            }
            return -1;
        } else {
            TIHI_LOG_DEBUG(g_sys_logger) << "<" << hook_fun_name << ">";
            tihi::Fiber::YieldToHold();
            TIHI_LOG_DEBUG(g_sys_logger) << "<" << hook_fun_name << ">";
            if (timer) {
                timer->cancel();
            }
            if (tinfo->cancelled) {
                errno = tinfo->cancelled;
                return -1;
            }

            goto retry;
        }
    }

    return n;
}

int connect_with_timeout(int sockfd, const struct sockaddr *addr, socklen_t addrlen, uint64_t timeout) {
    if (!is_hook_enable()) {
        return connect_f(sockfd, addr, addrlen);
    }

    // TIHI_LOG_DEBUG(g_sys_logger) << "directly return";
    // return 1;

    FdCtx::ptr fdctx = FdMgr::GetInstance()->fd(sockfd);
    if (!fdctx) {
        return connect_f(sockfd, addr, addrlen);
    }

    if (!fdctx->is_socket() || fdctx->user_nonblock()) {
        return connect_f(sockfd, addr, addrlen);
    }

    if (fdctx->is_closed()) {
        errno = EBADFD;
        return -1;
    }

    std::shared_ptr<timer_info> tinfo(new timer_info);

    int n = connect_f(sockfd, addr, addrlen);
    if (n == 0) {
        return 0;
    } else if (n != -1 || EINPROGRESS != errno) {
        return n;
    }
    
    tihi::IOManager *iom = tihi::IOManager::This();
    tihi::Timer::ptr timer;
    std::weak_ptr<timer_info> wtinfo(tinfo);

    if ((uint64_t)-1 != timeout) {
        timer = iom->addConditionTimer(
            timeout,
            [wtinfo, sockfd, iom]() {
                auto t = wtinfo.lock();
                if (!t || t->cancelled) {
                    return;
                }
                t->cancelled = ETIMEDOUT;
                iom->cancelEvent(sockfd, tihi::IOManager::WRITE);
            },
            wtinfo);
    }

    /**
     * 回调函数为空则以当前协程为调度对象
     */
    int rt = iom->addEvent(sockfd, tihi::IOManager::WRITE);
    if (rt) {
        TIHI_LOG_ERROR(g_sys_logger)
            << "connect addEvent(fd=" << sockfd << ", type=" << tihi::IOManager::WRITE
            << ")";
        if (timer) {
            timer->cancel();
        }
    } else {
        tihi::Fiber::YieldToHold();
        if (timer) {
            timer->cancel();
        }
        if (tinfo->cancelled) {
            errno = tinfo->cancelled;
            return -1;
        }
    }

    int error = 0;
    socklen_t len = sizeof(int);
    if (-1 == getsockopt(sockfd, SOL_SOCKET, SO_ERROR, &error, &len)) {
        return -1;
    }

    if (error) {
        errno = error;
    }

    return error;
}

}  // namespace tihi

extern "C" {
#define XX(name) name##_fun name##_f = NULL;

HOOK_FUN(XX);

#undef XX

unsigned int sleep(unsigned int seconds) {
    if (!tihi::is_hook_enable()) {
        return sleep_f(seconds);
    }

    tihi::Fiber::ptr fiber = tihi::Fiber::This();
    tihi::IOManager *iom = tihi::IOManager::This();
    iom->addTimer(
        seconds * 1000,
        std::bind((void (tihi::Scheduler::*)(tihi::Fiber::ptr, pid_t))(
                      &tihi::IOManager::schedule),
                  iom, fiber, -1));
    // iom->addTimer(seconds * 1000, [iom, fiber]() { iom->schedule(fiber); });
    tihi::Fiber::YieldToHold();
    return 0;
}

int usleep(useconds_t usec) {
    if (!tihi::is_hook_enable()) {
        return usleep_f(usec);
    }

    tihi::Fiber::ptr fiber = tihi::Fiber::This();
    tihi::IOManager *iom = tihi::IOManager::This();
    iom->addTimer(
        usec / 1000,
        std::bind((void (tihi::IOManager::*)(tihi::Fiber::ptr, pid_t))(
                      &tihi::IOManager::schedule),
                  iom, fiber, -1));
    // iom->addTimer(usec / 1000, [iom, fiber]() { iom->schedule(fiber); });
    tihi::Fiber::YieldToHold();
    return 0;
}

int nanosleep(const struct timespec *req, struct timespec *rem) {
    if (!tihi::is_hook_enable()) {
        return nanosleep_f(req, rem);
    }

    uint64_t ms = req->tv_sec * 1000 + req->tv_nsec / 1000 / 1000;

    tihi::Fiber::ptr fiber = tihi::Fiber::This();
    tihi::IOManager *iom = tihi::IOManager::This();
    iom->addTimer(
        ms, std::bind((void (tihi::Scheduler::*)(tihi::Fiber::ptr, pid_t))(
                          &tihi::IOManager::schedule),
                      iom, fiber, -1));
    // iom->addTimer(ms, [iom, fiber]() { iom->schedule(fiber); });
    tihi::Fiber::YieldToHold();
    return 0;
}

int socket(int domain, int type, int protocol) {
    if (!tihi::is_hook_enable()) {
        return socket_f(domain, type, protocol);
    }

    int fd = socket_f(domain, type, protocol);
    if (-1 == fd) {
        return fd;
    }
    tihi::FdMgr::GetInstance()->fd(fd, true);
    return fd;
}

int connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen) {
    return tihi::connect_with_timeout(sockfd, addr, addrlen, tihi::s_tcp_connect_timeout);
}

int accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen) {
    int fd = tihi::do_io(sockfd, accept_f, "accept", tihi::IOManager::READ,
                         SO_RCVTIMEO, addr, addrlen);
    if (fd >= 0) {
        tihi::FdMgr::GetInstance()->fd(fd, true);
    }
    return fd;
}

ssize_t read(int fd, void *buf, size_t count) {
    return tihi::do_io(fd, read_f, "read", tihi::IOManager::READ, SO_RCVTIMEO,
                       buf, count);
}

ssize_t readv(int fd, const struct iovec *iov, int iovcnt) {
    return tihi::do_io(fd, readv_f, "readv", tihi::IOManager::READ, SO_RCVTIMEO,
                       iov, iovcnt);
}

ssize_t recv(int sockfd, void *buf, size_t len, int flags) {
    return tihi::do_io(sockfd, recv_f, "recv", tihi::IOManager::READ,
                       SO_RCVTIMEO, buf, len, flags);
}

ssize_t recvfrom(int sockfd, void *buf, size_t len, int flags,
                 struct sockaddr *src_addr, socklen_t *addrlen) {
    return tihi::do_io(sockfd, recvfrom_f, "recvfrom", tihi::IOManager::READ,
                       SO_RCVTIMEO, buf, len, flags, src_addr, addrlen);
}

ssize_t recvmsg(int sockfd, struct msghdr *msg, int flags) {
    return tihi::do_io(sockfd, recvmsg_f, "readmsg", tihi::IOManager::READ,
                       SO_RCVTIMEO, msg, flags);
}

ssize_t write(int fd, const void *buf, size_t count) {
    return tihi::do_io(fd, write_f, "write", tihi::IOManager::WRITE,
                       SO_SNDTIMEO, buf, count);
}

ssize_t writev(int fd, const struct iovec *iov, int iovcnt) {
    return tihi::do_io(fd, writev_f, "writev", tihi::IOManager::WRITE,
                       SO_SNDTIMEO, iov, iovcnt);
}

ssize_t send(int sockfd, const void *buf, size_t len, int flags) {
    return tihi::do_io(sockfd, send_f, "send", tihi::IOManager::WRITE,
                       SO_SNDTIMEO, buf, len, flags);
}

ssize_t sendto(int sockfd, const void *buf, size_t len, int flags,
               const struct sockaddr *dest_addr, socklen_t addrlen) {
    return tihi::do_io(sockfd, sendto_f, "sendto", tihi::IOManager::WRITE,
                       SO_SNDTIMEO, buf, len, flags, dest_addr, addrlen);
}

ssize_t sendmsg(int sockfd, const struct msghdr *msg, int flags) {
    return tihi::do_io(sockfd, sendmsg_f, "sendmsg", tihi::IOManager::WRITE,
                       SO_SNDTIMEO, msg, flags);
}

int close(int fd) {
    if (!tihi::is_hook_enable()) {
        return close_f(fd);
    }

    tihi::FdCtx::ptr ctx = tihi::FdMgr::GetInstance()->fd(fd);
    if (ctx) {
        tihi::IOManager *iom = tihi::IOManager::This();
        if (iom) {
            iom->cancelAll(fd);
        }
        tihi::FdMgr::GetInstance()->delFd(fd);
    }

    return close_f(fd);
}

int fcntl(int fd, int cmd, ... /* arg */) {
    va_list vl;
    va_start(vl, cmd);
    switch (cmd) {
        case F_SETFL: {
            int arg = va_arg(vl, int);
            va_end(vl);
            tihi::FdCtx::ptr ctx = tihi::FdMgr::GetInstance()->fd(fd);
            if (!ctx || ctx->is_closed() || !ctx->is_socket()) {
                return fcntl_f(fd, cmd, arg);
            }
            ctx->set_user_nonblock(arg & O_NONBLOCK);
            if (ctx->sys_nonblock()) {
                arg |= O_NONBLOCK;
            } else {
                arg &= ~O_NONBLOCK;
            }
            return fcntl_f(fd, cmd, arg);
        } break;
        case F_GETFL: {
            va_end(vl);
            int arg = fcntl_f(fd, cmd);
            tihi::FdCtx::ptr ctx = tihi::FdMgr::GetInstance()->fd(fd);
            if (!ctx || ctx->is_closed() || !ctx->is_socket()) {
                return arg;
            }
            if (ctx->user_nonblock()) {
                return arg | O_NONBLOCK;
            } else {
                return arg & ~O_NONBLOCK;
            }
            return arg;
        } break;
        case F_DUPFD:
        case F_DUPFD_CLOEXEC:
        case F_SETFD:
        case F_SETOWN:
        case F_SETSIG:
        case F_SETLEASE:
        case F_NOTIFY:
        case F_SETPIPE_SZ: {
            int arg = va_arg(vl, int);
            va_end(vl);
            return fcntl_f(fd, cmd, arg);
        } break;
        case F_GETFD:
        case F_GETOWN:
        case F_GETSIG:
        case F_GETLEASE:
        case F_GETPIPE_SZ: {
            va_end(vl);
            return fcntl_f(fd, cmd);
        } break;
        case F_GETLK:
        case F_SETLK:
        case F_SETLKW: {
            struct flock *arg = va_arg(vl, struct flock *);
            va_end(vl);
            return fcntl_f(fd, cmd, arg);
        } break;
        case F_GETOWN_EX:
        case F_SETOWN_EX: {
            struct f_owner_ex *arg = va_arg(vl, struct f_owner_ex *);
            va_end(vl);
            return fcntl_f(fd, cmd, arg);
        } break;
        default:
            va_end(vl);
            return fcntl_f(fd, cmd);
            break;
    }

    return -1;
}

int ioctl(int d, long unsigned int request, ...) {
    va_list vl;
    va_start(vl, request);
    void* arg = va_arg(vl, void*);
    va_end(vl);

    if (FIONBIO == request) {
        tihi::FdCtx::ptr ctx = tihi::FdMgr::GetInstance()->fd(d);
        if (!ctx || ctx->is_socket() || !ctx->is_socket()) {
            return ioctl_f(d, request, arg);
        }
        bool user_nonblock = !!*(int*)(arg);
        ctx->set_user_nonblock(user_nonblock);
    }

    return ioctl_f(d, request, arg);
}

int getsockopt(int sockfd, int level, int optname, void *optval,
               socklen_t *optlen) {
    return getsockopt_f(sockfd, level, optname, optval, optlen);
}

int setsockopt(int sockfd, int level, int optname, const void *optval,
               socklen_t optlen) {
    if (!tihi::is_hook_enable()) {
        return setsockopt_f(sockfd, level, optname, optval, optlen);
    }

    if (SOL_SOCKET == level) {
        if (SO_RCVTIMEO == optname || SO_SNDTIMEO == optname) {
            tihi::FdCtx::ptr ctx = tihi::FdMgr::GetInstance()->fd(sockfd);
            if (ctx) {
                const struct timeval* tv = (const timeval*)optval;
                ctx->set_timeout(level, tv->tv_sec * 1000 + tv->tv_usec / 1000);
            }
        }
    }

    return setsockopt_f(sockfd, level, optname, optval, optlen);
}

}  // extern "C"