#include <ledctrl/server_base.h>
#include <iostream>

using namespace mega_camera;

/*LedServer server(8014,
    {1, 1, 1},

    [](DataBuffer data, LedServer::Client& client){
    },

    [](LedServer::Client& client) {
      std::cout << "Client connected\n";
    },

    [](LedServer::Client& client) {
      std::cout << "Client disconnected\n";
    },

    std::thread::hardware_concurrency()
);*/

LedServer server(8014);

int main()
{
    try
    {
        if (server.start() == LedServer::SocketStatus::up) {
            server.joinLoop();
            return EXIT_SUCCESS;
        }
        else
        {
            std::cout << "Server start error! Error code:"
                      << int(server.getStatus()) << std::endl;
            return EXIT_FAILURE;
        }
    }
    catch (std::exception& except)
    {
        std::cerr << except.what();
        return EXIT_FAILURE;
    }
}
