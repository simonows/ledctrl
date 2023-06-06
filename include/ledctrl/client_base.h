#ifndef __LEDCLIENT_H__
#define __LEDCLIENT_H__

#include <ledctrl/general.h>

#include <cstdint>
#include <cstddef>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>

typedef int socket_t;

namespace mega_camera
{

class LedClient : public LedClientBase
{
    SocketAddr_in address;
    socket_t client_socket;

    std::mutex handle_mutex;
    std::function<void(DataBuffer)> handler_func = [](DataBuffer) noexcept {};

    std::thread recv_thread;

    std::atomic<SocketStatus> _status;

    void handle_recv_thread();

public:
    typedef std::function<void(DataBuffer)> handler_function_t;

    LedClient() noexcept :
        address()
      , client_socket()
      , handle_mutex()
      , recv_thread()
      , _status(SocketStatus::disconnected)
    {}

    LedClient(const mega_camera::LedClient&) = delete;
    LedClient& operator=(const mega_camera::LedClient&) = delete;

    virtual ~LedClient() override;

    SocketStatus connectTo(const std::string&, uint16_t port) noexcept;
    virtual SocketStatus disconnect() override;

    virtual SocketStatus getStatus() const override { return _status; }

    virtual DataBuffer loadData() override;
    void setHandler(handler_function_t handler);

    virtual bool sendData(std::string) const override;
    virtual SocketType getType() const override {return SocketType::client_socket;}
};

}

#endif // __LEDCLIENT_H__
