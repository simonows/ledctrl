/*!
 * \brief Описание клиента.
*/
#ifndef __LED_CLIENT_H__
#define __LED_CLIENT_H__

#include <ledctrl/general.h>

#include <cstdint>
#include <cstddef>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <memory.h>
#include <unistd.h>

namespace mega_camera {

/*!
 * \brief Класс клиента.
 *
 * Необходимое описание на стороне клиента. Объект этого класса позволяет
 * подключать сервер.
*/
class LedClient : public LedClientBase {
    SocketAddr_in address;
    std::mutex handle_mutex;
    std::function<void(DataBuffer)> handler_func = [](
    DataBuffer) noexcept {};
    std::thread recv_thread;
    void handle_recv_thread();

  public:
    typedef std::function<void(DataBuffer)>
    handler_function_t;

    LedClient() noexcept :
        address()
      , handle_mutex()
      , recv_thread()
    {}

    LedClient(const mega_camera::LedClient&) = delete;
    LedClient& operator=(const
                         mega_camera::LedClient&) = delete;

    virtual ~LedClient() override;

    SocketStatus connectTo(const std::string&,
                           uint16_t port) noexcept;
    virtual SocketStatus disconnect() override;
    void setHandler(handler_function_t handler);
    virtual SocketStatus getStatus() const override {
        return _status;
    }
    virtual SocketType getType() const override {
        return SocketType::client_socket;
    }
};

}

#endif // __LED_CLIENT_H__
