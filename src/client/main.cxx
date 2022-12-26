#include <ledctrl/client_base.h>

#include <iostream>
#include <stdlib.h>
#include <thread>

using namespace mega_camera;

void run_client(LedClient& client)
{
    using namespace std::chrono_literals;

    if (client.connectTo(LOCALHOST_IP, 8014) != SocketStatus::connected)
    {
        std::cerr << "Client isn't connected\n";
        std::exit(EXIT_FAILURE);
    }

    std::clog << "Client connected\n";
    client.setHandler([&client](DataBuffer data)
        {
            std::clog << "Recived " << data.size() << " bytes: "
                << reinterpret_cast<char*>(data.data()) << '\n';
        }
    );
    client.sendData("set-led-state off\n");
    client.sendData("get-led-state\n");
    client.sendData("set-led-color green\n");
    client.sendData("get-led-color\n");
    client.sendData("set-led-rate 3\n");
    client.sendData("get-led-rate 3\n");
}

int main(int, char**)
{
    LedClient client;
    run_client(client);
    client.joinHandler();

    return EXIT_SUCCESS;
}
