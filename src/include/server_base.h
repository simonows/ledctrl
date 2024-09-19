/*!
 * \brief Определение сервера.
*/
#ifndef __LED_SERVER_H__
#define __LED_SERVER_H__

#include <ledctrl/general.h>
#include "thread_pool.h"

#include <functional>
#include <list>
#include <thread>
#include <mutex>
#include <shared_mutex>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <unistd.h>
#include <fcntl.h>


namespace mega_camera {

/*!
 * \brief Класс сервера
 *
 * Серверный объект основан на пуле потоков и очереди заданий. По мере поступления задач,
 * они распределяются между потоками.
*/
struct LedServer {
    struct Client;

    typedef std::function<void(DataBuffer, Client&)>
    handler_function_t;
    typedef std::function<void(Client&)>
    con_handler_function_t;

    LedServer(
        const uint16_t port,
        KeepAliveConfig ka_conf = {},
        handler_function_t handler = nullptr,
        con_handler_function_t connect_hndl =
            default_connection_handler,
        con_handler_function_t disconnect_hndl =
            default_connection_handler,
        uint thread_count =
            std::thread::hardware_concurrency()
    );

    ~LedServer();

    SocketStatus getStatus() const {
        return _status;
    }

    SocketStatus start();
    void stop();
    void joinLoop();
    static void print_screen(void);

  private:
    Socket serv_socket;
    ThreadPool thread_pool;

    uint16_t port;
    SocketStatus _status = SocketStatus::close;

    static constexpr auto default_connection_handler
    = [](Client&) noexcept {};

    handler_function_t handler;
    con_handler_function_t connect_hndl =
        default_connection_handler;
    con_handler_function_t disconnect_hndl =
        default_connection_handler;

    KeepAliveConfig ka_conf;
    std::list<std::unique_ptr<Client>> client_list;
    std::mutex client_mutex;

    bool enableKeepAlive(Socket socket);
    void handlingAcceptLoop();
    void waitingDataLoop();

    void server_business(DataBuffer,
                         LedServer::Client&);
};

/*!
 * \brief Определение клиента на стороне сервера.
*/
struct LedServer::Client : public LedClientBase {
    friend struct LedServer;

    std::mutex access_mtx;
    SocketAddr_in address;

    Client(Socket socket, SocketAddr_in address);
    virtual ~Client() override;
    virtual mega_camera::SocketStatus getStatus()
    const override {
        return _status;
    }
    virtual mega_camera::SocketStatus disconnect()
    override;

    virtual SocketType getType() const override {
        return SocketType::server_socket;
    }
};

}

#endif // __LED_SERVER_H__
