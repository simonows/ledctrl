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
            else if (_status != SocketStatus::connected) return;
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

    if (recv_thread) recv_thread->join();
    delete recv_thread;
}

SocketStatus LedClient::connectTo(uint32_t host, uint16_t port) noexcept
{
    if ((client_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_IP)) < 0)
    {
        return _status = SocketStatus::err_socket_init;
    }

    new(&address) SocketAddr_in;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = host;
    address.sin_port = htons(port);

    if (connect(client_socket, reinterpret_cast<sockaddr *>(&address), sizeof(address)) != 0)
    {
        close(client_socket);
        return _status = SocketStatus::err_socket_connect;
    }
    return _status = SocketStatus::connected;
}

SocketStatus LedClient::disconnect() noexcept
{
    if (_status != SocketStatus::connected)
    {
        return _status;
    }

    _status = SocketStatus::disconnected;

    if (recv_thread) recv_thread->join();
    delete recv_thread;

    shutdown(client_socket, SD_BOTH);
    close(client_socket);
    return _status;
}

DataBuffer LedClient::loadData()
{
    DataBuffer buffer;
    uint32_t size;
    int err;

    int answ = recv(client_socket, reinterpret_cast<char*>(&size), sizeof(size), MSG_DONTWAIT);

    if (!answ)
    {
        disconnect();
        return DataBuffer();
    }
    else if (answ == -1)
    {
        SockLen_t len = sizeof (err);
        getsockopt (client_socket, SOL_SOCKET, SO_ERROR, &err, &len);
        if (!err) err = errno;

        switch (err)
        {
            case 0:
                break;
            case ETIMEDOUT:
            case ECONNRESET:
            case EPIPE:
                disconnect();
                [[fallthrough]];
            case EAGAIN:
                return DataBuffer();
            default:
                disconnect();
                return DataBuffer();
        }
    }

    if (!size)
    {
        return DataBuffer();
    }
    buffer.resize(size);
    recv(client_socket, reinterpret_cast<char*>(buffer.data()), buffer.size(), 0);
    return buffer;
}

void LedClient::setHandler(LedClient::handler_function_t handler)
{
    {
        std::lock_guard lock(handle_mutex);
        handler_func = handler;
    }

    if (recv_thread) return;
    recv_thread = new std::thread(&LedClient::handle_recv_thread, this);
}

void LedClient::joinHandler()
{
    if (recv_thread)
    {
        recv_thread->join();
    }
}

bool LedClient::sendData(const void* buffer, const size_t size) const
{
    void* send_buffer = malloc(size + sizeof(int));

    memcpy(reinterpret_cast<char*>(send_buffer) + sizeof(int), buffer, size);
    *reinterpret_cast<int*>(send_buffer) = size;
    if (send(client_socket, reinterpret_cast<char*>(send_buffer), size +
        sizeof(int), 0) < 0) return false;

    free(send_buffer);
    return true;
}
