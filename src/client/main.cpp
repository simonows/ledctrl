/*!
 * \brief Тест клиента.
*/
#include <ledctrl/client.h>

#include <iostream>
#include <thread>
#include <chrono>
#include <thread>

static const std::string LOCALHOST_IP =
    "127.0.0.1";

using namespace mega_camera;


void run_client(LedClient& client) {
    using namespace std::chrono_literals;

    if (client.connectTo(LOCALHOST_IP,
                         8014) != SocketStatus::connected) {
        std::cerr << "Client isn't connected\n";
        std::exit(EXIT_FAILURE);
    }

    std::clog << "Client connected\n";
    client.setHandler(
        [&client](DataBuffer data) {
            std::clog << "Recived " << data.size() << " bytes" << std::endl;
        }
    );
    client.sendData("set-led-state off\n");
    client.sendData("get-led-state\n");
    client.sendData("set-led-color green\n");
    client.sendData("get-led-color\n");
    client.sendData("set-led-rate 3\n");
    client.sendData("get-led-rate 3\n");
    // Чтобы успеть принять ответы
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    client.disconnect();
}

int main(int, char**) {
    LedClient client;
    run_client(client);
    return EXIT_SUCCESS;
}
