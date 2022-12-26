#include <ledctrl/server_base.h>

#include <iostream>
#include <chrono>
#include <cstring>
#include <mutex>

using namespace mega_camera;


LedServer::LedServer(
    const uint16_t _port
  , KeepAliveConfig _ka_conf
  , handler_function_t _handler
  , con_handler_function_t _connect_hndl
  , con_handler_function_t _disconnect_hndl
  , uint _thread_count
) : serv_socket()
  , port(_port)
  , handler(_handler)
  , connect_hndl(_connect_hndl)
  , disconnect_hndl(_disconnect_hndl)
  , thread_pool(_thread_count)
  , ka_conf(_ka_conf)
  , client_list()
  , client_mutex()
{}

LedServer::~LedServer()
{
    if (_status == SocketStatus::up)
        stop();
}

LedServer::SocketStatus LedServer::start()
{
    int flag{true};
    SocketAddr_in address;

    if (_status == SocketStatus::up)
    {
        stop();
    }

    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);
    address.sin_family = AF_INET;

    if ((serv_socket = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0)) == -1)
    {
        return _status = SocketStatus::err_socket_init;
    }

    bool p1 = setsockopt(serv_socket, SOL_SOCKET, SO_REUSEADDR, &flag,
        sizeof(flag)) == -1;
    bool p2 = bind(serv_socket, reinterpret_cast<struct sockaddr*>(&address),
        sizeof(address)) < 0;

    if (p1 || p2)
    {
        return _status = SocketStatus::err_socket_bind;
    }

    if (listen(serv_socket, SOMAXCONN) < 0)
    {
        return _status = SocketStatus::err_socket_listening;
    }

    print_screen();
    _status = SocketStatus::up;

    thread_pool.addJob([this]{handlingAcceptLoop();});
    thread_pool.addJob([this]{waitingDataLoop();});

    return _status;
}

void LedServer::stop()
{
    thread_pool.dropUnstartedJobs();
    _status = SocketStatus::close;
    close(serv_socket);
    client_list.clear();
}

void LedServer::joinLoop()
{
    thread_pool.join();
}

void LedServer::handlingAcceptLoop()
{
    SockLen_t addrlen = sizeof(SocketAddr_in);
    SocketAddr_in client_addr;
    Socket client_socket = accept4(
        serv_socket
      , reinterpret_cast<struct sockaddr*>(&client_addr)
      , &addrlen
      , SOCK_NONBLOCK
    );

    if (client_socket >= 0 && _status == SocketStatus::up)
    {
        if (enableKeepAlive(client_socket))
        {
            std::unique_ptr<Client> client(new Client(client_socket, client_addr));
            connect_hndl(*client);
            client_mutex.lock();
            client_list.emplace_back(std::move(client));
            client_mutex.unlock();
        }
        else
        {
            shutdown(client_socket, 0);
            close(client_socket);
        }
    }

    if (_status == SocketStatus::up)
    {
        thread_pool.addJob([this](){handlingAcceptLoop();});
    }
}

void LedServer::waitingDataLoop()
{
    {
        std::lock_guard lock(client_mutex);

        for (auto it = client_list.begin(), end = client_list.end(); it != end; ++it)
        {
            auto& client = *it;

            if (not client)
            {
                continue;
            }

            if (DataBuffer data = client->loadData(); not data.empty())
            {
                thread_pool.addJob(
                    [this, _data = std::move(data), &client]
                    {
                        client->access_mtx.lock();
                        if (handler) handler(std::move(_data), *client);
                        else server_business(std::move(_data), *client);
                        client->access_mtx.unlock();
                    }
                );
            }
            else if (client->_status == mega_camera::SocketStatus::disconnected)
            {
                thread_pool.addJob(
                    [this, &client, it]
                    {
                        client->access_mtx.lock();
                        Client* pointer = client.release();
                        client = nullptr;
                        pointer->access_mtx.unlock();
                        disconnect_hndl(*pointer);
                        client_list.erase(it);
                        delete pointer;
                    }
                );
            }
        }
    }

    if (_status == SocketStatus::up)
    {
        thread_pool.addJob([this](){ waitingDataLoop(); });
    }
}

bool LedServer::enableKeepAlive(Socket socket)
{
    int flag{1};

    if (setsockopt(
        socket
      , SOL_SOCKET
      , SO_KEEPALIVE
      , &flag
      , sizeof(flag)
    ) == -1) return false;
    if (setsockopt(
        socket
      , IPPROTO_TCP
      , TCP_KEEPIDLE
      , &ka_conf.ka_idle
      , sizeof(ka_conf.ka_idle)
    ) == -1) return false;
    if (setsockopt(
        socket
      , IPPROTO_TCP
      , TCP_KEEPINTVL
      , &ka_conf.ka_intvl
      , sizeof(ka_conf.ka_intvl)
    ) == -1) return false;
    if (setsockopt(
        socket
      , IPPROTO_TCP
      , TCP_KEEPCNT
      , &ka_conf.ka_cnt
      , sizeof(ka_conf.ka_cnt)
    ) == -1) return false;

    return true;
}

DataBuffer LedServer::Client::loadData()
{
    DataBuffer buffer;
    uint32_t size;
    int err;

    if (_status != mega_camera::SocketStatus::connected)
    {
        return DataBuffer();
    }

    using namespace std::chrono_literals;

    // non-blocking mode
    int answ = recv(socket, reinterpret_cast<char*>(&size), sizeof(size), MSG_DONTWAIT);

    if (not answ)
    {
        disconnect();
        return DataBuffer();
    }
    else if (answ == -1)
    {
        // Error handle
        SockLen_t len = sizeof (err);
        getsockopt (socket, SOL_SOCKET, SO_ERROR, &err, &len);
        if (!err)
            err = errno;

        switch (err)
        {
        case 0:
            break;
            // Keep alive timeout
        case ETIMEDOUT:
        case ECONNRESET:
        case EPIPE:
            disconnect();
            [[fallthrough]];
            // No data
        case EAGAIN:
            return DataBuffer();
        default:
            disconnect();
            std::cerr << "Unhandled error!\n"
                << "Code: " << err << " Err: " << std::strerror(err) << '\n';
            return DataBuffer();
        }
    }

    if (not size)
    {
        return DataBuffer();
    }

    buffer.resize(size);
    recv(socket, buffer.data(), buffer.size(), 0);

    return buffer;
}


mega_camera::SocketStatus LedServer::Client::disconnect()
{
    _status = mega_camera::SocketStatus::disconnected;

    if (socket == -1)
    {
        return _status;
    }

    shutdown(socket, SD_BOTH);
    close(socket);
    socket = -1;

    return _status;
}

bool LedServer::Client::sendData(std::string str) const
{
    const char *buffer = str.data();
    size_t size = str.length();

    if (_status != mega_camera::SocketStatus::connected)
    {
        return false;
    }

    void* send_buffer = malloc(size + sizeof (uint32_t));
    memcpy(reinterpret_cast<char*>(send_buffer) + sizeof(uint32_t), buffer, size);
    *reinterpret_cast<uint32_t*>(send_buffer) = size;

    if (send(socket, reinterpret_cast<char*>(send_buffer), size + sizeof (int), 0) < 0)
    {
        return false;
    }

    free(send_buffer);
    return true;
}


LedServer::Client::Client(Socket _socket, SocketAddr_in _address)
  : access_mtx(), address(_address), socket(_socket) {}


LedServer::Client::~Client()
{
    if (socket == -1) return;
    shutdown(socket, SD_BOTH);
    close(socket);
}
