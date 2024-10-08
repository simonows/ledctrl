/*!
 * \brief Реализация клиента.
*/
#include <ledctrl/client.h>

#include <stdio.h>
#include <cstring>
#include <iostream>

using namespace mega_camera;

/*!
 * Обработка данных поступающих из сокета.
*/
void LedClient::handle_recv_thread() {
    try {
        while (_status == SocketStatus::connected) {
            if (DataBuffer data = loadData(); not data.empty()) {
                std::lock_guard lock(handle_mutex);
                handler_func(std::move(data));
            }
        }
    } catch (std::exception& except) {
        std::cerr << except.what() << std::endl;
    }
}

/*!
 * Соединение с сервером.
 *
 * \param[in] host Адрес.
 * \param[in] port Порт.
 * \return Состояние сокета.
*/
SocketStatus LedClient::connectTo(const std::string &host, const uint16_t port) noexcept {
    if ((_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_IP)) < 0)
        return _status = SocketStatus::err_socket_init;

    new(&address) SocketAddr_in;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = inet_addr(host.c_str());
    address.sin_port = htons(port);

    if (connect(_socket, reinterpret_cast<sockaddr *>(&address), sizeof(address)) != 0) {
        close(_socket);
        return _status = SocketStatus::err_socket_connect;
    }
    return _status = SocketStatus::connected;
}

/*!
 * \brief Отключение от сервера.
 *
 * Используется блокирующий сокет. Он блокирует recv, но после закрытия recv
 * возвращает ноль, который обрабатывается в соответствии со статусом и приводит к отключению.
 *
 * \return Состояние сокета.
*/
SocketStatus LedClient::disconnect() {
    if (_status == SocketStatus::disconnected ||
            _status == SocketStatus::err_socket_connect)
        return _status;

    try {
        _status = SocketStatus::disconnected;
        shutdown(_socket, SD_BOTH);
        if (recv_thread.joinable()) recv_thread.join();
        close(_socket);
    } catch (std::exception& except) {
        std::cerr << except.what() << std::endl;
    }
    return static_cast<SocketStatus>(_status);
}

/*!
 * \brief Прием данных.
 *
 * \return buffer, .size() == 0 иначе.
*/
DataBuffer LedClientBase::loadData() {
    DataBuffer buffer;
    uint32_t size;
    int err;

    //! Blocking mode.
    int answ = recv(_socket, &size, sizeof(size), 0);

    if (answ == 0) {
        _status = SocketStatus::disconnected;
        return DataBuffer();
    } else if (answ == -1) {
        SockLen_t len = sizeof(err);
        getsockopt(_socket, SOL_SOCKET, SO_ERROR, &err,
                   &len);
        if (!err)
            err = errno;

        switch (err) {
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

    if (size > MAX_MESSAGE_SIZE)
        return DataBuffer();

    buffer.resize(size);
    answ = recv(_socket, buffer.data(), buffer.size(), 0);

    return answ > 0 ? buffer : DataBuffer();
}

/*!
 * \brief Отправка данных.
 *
 * \param[in] str Данные для отправки.
*/
bool LedClientBase::sendData(const std::string str) const noexcept {
    const char *buffer = str.data();
    uint32_t size = str.length();
    char send_buffer[MAX_MESSAGE_SIZE];

    if (size > MAX_MESSAGE_SIZE - sizeof(uint32_t) ||
            _status != mega_camera::SocketStatus::connected)
        return false;

    memcpy(send_buffer + sizeof(uint32_t), buffer, size);
    memcpy(send_buffer, &size, sizeof(uint32_t));

    if (send(_socket, send_buffer, size + sizeof(uint32_t), MSG_NOSIGNAL) < 0)
        return false;

    return true;
}

/*!
 * \brief Установка обработчика на прием.
 *
 * \param[in] handler Функция обработчика.
*/
void LedClient::setHandler(const LedClient::handler_function_t handler) {
    {
        std::lock_guard lock(handle_mutex);
        handler_func = handler;
    }

    std::thread th = std::thread(&LedClient::handle_recv_thread, this);
    recv_thread = std::move(th);
}

LedClient::~LedClient() {
    disconnect();
}
