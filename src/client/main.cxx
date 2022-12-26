#include <ledctrl/client_base.h>

#include <iostream>
#include <stdlib.h>
#include <thread>

using namespace mega_camera;

std::string getHostStr(uint32_t ip, uint16_t port)
{
    return std::string()
      + std::to_string(int(reinterpret_cast<char*>(&ip)[0])) + '.'
      + std::to_string(int(reinterpret_cast<char*>(&ip)[1])) + '.'
      + std::to_string(int(reinterpret_cast<char*>(&ip)[2])) + '.'
      + std::to_string(int(reinterpret_cast<char*>(&ip)[3])) + ':'
      + std::to_string(static_cast<int>(port));
}

void runClient(LedClient& client)
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
            std::this_thread::sleep_for(1s);
            client.sendData("Hello, server\0", sizeof ("Hello, server\0"));
        }
    );
    client.sendData("Hello, server\0", sizeof("Hello, server\0"));
    client.sendData("set-led-state pizda suka\0", sizeof("set-led-state pizda suka\0"));
}

int main(int, char**) {
  ThreadPool thread_pool;

  LedClient first_client(&thread_pool);
  LedClient second_client(&thread_pool);
  LedClient thrird_client(&thread_pool);
  LedClient fourth_client(&thread_pool);

  runClient(first_client);
  runClient(second_client);
  runClient(thrird_client);
  runClient(fourth_client);

  first_client.joinHandler();
  second_client.joinHandler();
  thrird_client.joinHandler();
  fourth_client.joinHandler();

  return EXIT_SUCCESS;
}
