/*!
 * \brief Common definitions of the application.
*/
#ifndef __GENERAL_H__
#define __GENERAL_H__

#define SD_BOTH 0

#include <sys/socket.h>
#include <netinet/in.h>

#include <cstdint>
#include <cstring>
#include <cinttypes>
#include <malloc.h>

#include <vector>
#include <functional>
#include <thread>
#include <condition_variable>
#include <atomic>

namespace mega_camera
{

static const size_t MAX_MESSAGE_SIZE = 65536;

typedef socklen_t SockLen_t;
typedef struct sockaddr_in SocketAddr_in;
typedef int Socket;
typedef int ka_prop_t;
typedef std::vector<uint8_t> DataBuffer;

//! Keep alive settings.
struct KeepAliveConfig
{
    ka_prop_t ka_idle = 100;
    ka_prop_t ka_intvl = 5;
    ka_prop_t ka_cnt = 10;
};

//! A set of possible socket states.
enum class SocketStatus : uint8_t
{
    connected = 0,
    err_socket_init,
    err_socket_bind,
    err_socket_connect,
    err_socket_read,
    err_socket_unused,
    err_scoket_keep_alive,
    err_socket_listening,
    disconnected,
    up,
    close
};

//! A set of possible socket types.
enum class SocketType : uint8_t
{
    client_socket = 0,
    server_socket
};

//! Base class for client and server.
struct LedClientBase
{
    typedef SocketStatus status;
    virtual ~LedClientBase() {};
    virtual SocketStatus disconnect() = 0;
    virtual SocketStatus getStatus() const = 0;
    virtual SocketType getType() const = 0;
    DataBuffer loadData();
    bool sendData(std::string) const noexcept;
    LedClientBase(): _socket(-1), _status(SocketStatus::close){}

protected:
    Socket _socket;
    std::atomic<SocketStatus> _status;
};

}

#endif // __GENERAL_H__
