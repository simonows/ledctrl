#include <ledctrl/client_base.h>

#include <stdio.h>
#include <cstring>
#include <iostream>


using namespace mega_camera;

void LedClient::handle_recv_thread()
{
    try
    {
        while (_status == SocketStatus::connected)
        {
            if (DataBuffer data = loadData(); not data.empty())
            {
                std::lock_guard lock(handle_mutex);
                handler_func(std::move(data));
            }
        }
    }
    catch (std::exception& except)
    {
        std::cerr << except.what() << std::endl;
    }
}

LedClient::~LedClient()
{
    disconnect();
}

SocketStatus LedClient::connectTo(const std::string &host, const uint16_t port) noexcept
{
    if ((client_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_IP)) < 0)
        return _status = SocketStatus::err_socket_init;

    new(&address) SocketAddr_in;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = inet_addr(host.c_str());
    address.sin_port = htons(port);

    if (connect(client_socket, reinterpret_cast<sockaddr *>(&address), sizeof(address)) != 0)
    {
        close(client_socket);
        return _status = SocketStatus::err_socket_connect;
    }
    return _status = SocketStatus::connected;
}

SocketStatus LedClient::disconnect()
{
    if (_status == SocketStatus::disconnected ||
        _status == SocketStatus::err_socket_connect)
        return _status;

    _status = SocketStatus::disconnected;
    shutdown(client_socket, SD_BOTH);
    recv_thread.join();
    close(client_socket);

    return static_cast<SocketStatus>(_status);
}

DataBuffer LedClient::loadData()
{
    DataBuffer buffer;
    uint32_t size;
    int err;

    int answ = recv(client_socket, &size, sizeof(size), 0);

    if (answ == 0)
    {
        return DataBuffer();
    }
    else if (answ == -1)
    {
        SockLen_t len = sizeof(err);
        getsockopt(client_socket, SOL_SOCKET, SO_ERROR, &err, &len);
        if (!err)
            err = errno;

        switch (err)
        {
            case 0:
                break;
            case ETIMEDOUT:
            case ECONNRESET:
            case EPIPE:
                _status = SocketStatus::err_socket_read;
                return DataBuffer();
            case EAGAIN:
                return DataBuffer();
            default:
                _status = SocketStatus::err_socket_unused;
                return DataBuffer();
        }
    }

    if (size > 65536)
        return DataBuffer();

    buffer.resize(size);
    answ = recv(client_socket, buffer.data(), buffer.size(), 0);

    return answ > 0 ? buffer : DataBuffer();
}

void LedClient::setHandler(const LedClient::handler_function_t handler)
{
    {
        std::lock_guard lock(handle_mutex);
        handler_func = handler;
    }

    std::thread th = std::thread(&LedClient::handle_recv_thread, this);
    recv_thread = std::move(th);
}


bool LedClient::sendData(const std::string str) const
{
    const char *buffer = str.data();
    size_t size = str.length();
    void* send_buffer = malloc(size + sizeof(int));

    memcpy(reinterpret_cast<char*>(send_buffer) + sizeof(int), buffer, size);
    *reinterpret_cast<int*>(send_buffer) = size;

    if (send(client_socket, reinterpret_cast<char*>(send_buffer), size + sizeof(int), 0) < 0)
        return false;

    free(send_buffer);
    return true;
}
