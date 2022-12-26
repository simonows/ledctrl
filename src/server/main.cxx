#include <ledctrl/server_base.h>
#include <iostream>

using namespace mega_camera;

/*LedServer server(8014,
{1, 1, 1}, // Keep alive setting

[](DataBuffer data, LedServer::Client& client){ // Data handler
  std::cout << "Client  send data [ " << data.size() << " bytes ]: " << reinterpret_cast<char*>(data.data()) << '\n';
  client.sendData("Hello, client!\0", sizeof("Hello, client!\0"));
},

[](LedServer::Client& client) { // Connect handler
  std::cout << "Client connected\n";
},

[](LedServer::Client& client) { // Disconnect handler
  std::cout << "Client disconnected\n";
},

std::thread::hardware_concurrency()
);*/

LedServer server(8014,
{1, 1, 1});

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
