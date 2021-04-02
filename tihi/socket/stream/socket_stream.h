#ifndef TIHI_SOCKET_STREAM_SOCKET_STREAM_H_
#define TIHI_SOCKET_STREAM_SOCKET_STREAM_H_

#include "stream.h"
#include "socket/socket/socket.h"

namespace tihi {

class SocketStream : public Stream{
public:
    using ptr = std::shared_ptr<SocketStream>;
    SocketStream(Socket::ptr socket, bool owner = true);
    ~SocketStream();

    virtual int read(void* buffer, size_t length) override;
    virtual int read(ByteArray::ptr ba, size_t length) override;
    virtual int write(const void* buffer, size_t length) override;
    virtual int write(ByteArray::ptr ba, size_t length) override;
    virtual void close() override;

    Socket::ptr socket() const { return m_socket; }
    bool isConnected() const;
private:
    Socket::ptr m_socket;
    bool m_owner;
};

} // namespace tihi

#endif // TIHI_SOCKET_STREAM_SOCKET_STREAM_H_