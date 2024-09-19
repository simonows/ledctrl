/*!
 * \brief Реализация сервера.
*/
#include "server_base.h"

#include <iostream>
#include <chrono>
#include <cstring>
#include <mutex>

using namespace mega_camera;

/*!
 * \brief Запуск сервера.
 *
 * \return Состояние сокета.
*/
SocketStatus LedServer::start() {
    int flag{true};
    SocketAddr_in address;

    if (_status == SocketStatus::up)
        return _status;

    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);
    address.sin_family = AF_INET;

    if ((serv_socket = socket(AF_INET, SOCK_STREAM, 0)) == -1)
        return _status = SocketStatus::err_socket_init;

    bool p1 = setsockopt(serv_socket, SOL_SOCKET,
                         SO_REUSEADDR, &flag,
                         sizeof(flag)) == -1;
    bool p2 = bind(serv_socket,
                   reinterpret_cast<struct sockaddr*>(&address),
                   sizeof(address)) < 0;

    if (p1 || p2)
        return _status = SocketStatus::err_socket_bind;

    if (listen(serv_socket, SOMAXCONN) < 0)
        return _status =
                   SocketStatus::err_socket_listening;

    print_screen();
    _status = SocketStatus::up;

    thread_pool.addJob([this] {handlingAcceptLoop();});
    thread_pool.addJob([this] {waitingDataLoop();});

    return _status;
}

/*!
 * \brief Остановка сервера.
 *
 * Используется блокирующий сокет. Сокет вызывает SIGPIPE в случае завершения работы. SIGPIPE
 * был отключен при обычном завершении работы. Также он блокирует recv, но после закрытия
 * recv возвращает ноль, который обрабатывается в соответствии со статусом и приводит к
 * отключению.
*/
void LedServer::stop() {
    _status = SocketStatus::close;
    shutdown(serv_socket, SD_BOTH);
    close(serv_socket);
    for (auto& cl : client_list)
        cl->disconnect();
    thread_pool.dropUnstartedJobs();
    client_list.clear();
}

/*!
 * \brief Простой join.
*/
void LedServer::joinLoop() {
    thread_pool.wait();
}

/*!
 * \brief Задание на прием новых соединений.
*/
void LedServer::handlingAcceptLoop() {
    SockLen_t addrlen = sizeof(SocketAddr_in);
    SocketAddr_in client_addr;

    //! Прием нового подключения в блокирующем режиме.
    Socket client_socket = accept4(
        serv_socket, reinterpret_cast<struct sockaddr*>(&client_addr), &addrlen, 0);

    if (client_socket >= 0 && _status == SocketStatus::up) {
        if (enableKeepAlive(client_socket)) {
            std::unique_ptr<Client> client(new Client(
                                               client_socket, client_addr));
            connect_hndl(*client);
            client_mutex.lock();
            client_list.emplace_back(std::move(client));
            client_mutex.unlock();
        } else {
            shutdown(client_socket, 0);
            close(client_socket);
        }
    }

    if (_status == SocketStatus::up)
        thread_pool.addJob([this]() {
        handlingAcceptLoop();
    });
}

/*!
 * \brief Задание на ожидание новых данных.
*/
void LedServer::waitingDataLoop() {
    {
        std::lock_guard lock(client_mutex);

        for (auto& client : client_list) {
            if (not client)
                continue;
            if (DataBuffer data = client->loadData(); not data.empty()) {
                thread_pool.addJob(
                    [this, _data = std::move(data), &client] {
                        client->access_mtx.lock();
                        if (handler) handler(std::move(_data), *client);
                        else server_business(std::move(_data), *client);
                        client->access_mtx.unlock();
                    }
                );
            } else if (client->_status == mega_camera::SocketStatus::disconnected) {
                thread_pool.addJob(
                    [this, &client] {
                        if (not client) return;
                        client->access_mtx.lock();
                        Client* pointer = client.release();
                        client = nullptr;
                        pointer->access_mtx.unlock();
                        disconnect_hndl(*pointer);
                        client_list.erase(
                            std::remove(client_list.begin(), client_list.end(), client),
                            client_list.end());
                        delete pointer;
                    }
                );
            }
        }
    }

    if (_status == SocketStatus::up) {
        while (client_list.size() == 0) {
            //! Сон, когда нет клиентов
            if (_status == SocketStatus::close)
                return;
            sleep(1);
        }
        thread_pool.addJob([this]() {
            waitingDataLoop();
        });
    }
}

/*!
 * \brief Включение KeepAlive параметров.
 *
 * \param[in] socket Сокет.
 * \return Статус операций.
*/
bool LedServer::enableKeepAlive(Socket socket) {
    int flag{1};

    if (setsockopt(socket, SOL_SOCKET, SO_KEEPALIVE,
                   &flag,
                   sizeof(flag)) == -1) return false;
    if (setsockopt(socket, IPPROTO_TCP, TCP_KEEPIDLE,
                   &ka_conf.ka_idle,
                   sizeof(ka_conf.ka_idle)) == -1) return false;
    if (setsockopt(socket, IPPROTO_TCP, TCP_KEEPINTVL,
                   &ka_conf.ka_intvl,
                   sizeof(ka_conf.ka_intvl)) == -1) return false;
    if (setsockopt(socket, IPPROTO_TCP, TCP_KEEPCNT,
                   &ka_conf.ka_cnt,
                   sizeof(ka_conf.ka_cnt)) == -1) return false;

    return true;
}

/*!
 * \brief Отключение клиента.
 *
 * \return Состояние сокета.
*/
mega_camera::SocketStatus
LedServer::Client::disconnect() {
    _status = mega_camera::SocketStatus::disconnected;

    if (_socket == -1)
        return _status;

    shutdown(_socket, SD_BOTH);
    close(_socket);
    _socket = -1;

    return _status;
}

LedServer::LedServer(
    const uint16_t _port
    , KeepAliveConfig _ka_conf
    , handler_function_t _handler
    , con_handler_function_t _connect_hndl
    , con_handler_function_t _disconnect_hndl
    , uint _thread_count
) : serv_socket(-1)
    , thread_pool(_thread_count)
    , port(_port)
    , handler(_handler)
    , connect_hndl(_connect_hndl)
    , disconnect_hndl(_disconnect_hndl)
    , ka_conf(_ka_conf)
    , client_list()
    , client_mutex()
{}


LedServer::Client::Client(Socket psocket,
                          SocketAddr_in _address)
    : access_mtx(), address(_address) {
    _socket = psocket;
    _status = SocketStatus::connected;
}

LedServer::~LedServer() {
    if (_status == SocketStatus::up)
        stop();
}

LedServer::Client::~Client() {
    if (_socket == -1)
        return;
    shutdown(_socket, SD_BOTH);
    close(_socket);
}
