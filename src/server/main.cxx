/*!
 * \brief Server's benchmark.
*/
#include <ledctrl/server_base.h>
#include <iostream>
#include <signal.h>

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

static void intHandler(int dummy)
{
    server.stop();
}

int main()
{
    struct sigaction act;
    act.sa_handler = &intHandler;
    sigfillset(&act.sa_mask);
    act.sa_flags = 0;

    if (sigaction(SIGINT, &act, NULL) == -1)
    {
        std::cout << "Error: Unable to handle SIGINT" << std::endl;
        return EXIT_FAILURE;
    }

    try
    {
        if (server.start() == LedServer::SocketStatus::up)
        {
            server.joinLoop();
            std::cout << std::endl << "Server stopped" << std::endl;
            _exit(EXIT_SUCCESS);
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
