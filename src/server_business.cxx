#include <ledctrl/server_base.h>
#include <iostream>
#include <map>

using namespace mega_camera;


typedef enum LedState : uint8_t
{
    ON = 0
  , OFF = 1
} LedState;

typedef enum LedColor : uint8_t
{
    RED = 0
  , GREEN = 1
  , BLUE = 2
} LedColor;

typedef uint8_t LedRate;
typedef void (*HandlerFuncPtr)(char const *);


void set_led_state(char const *);
void get_led_state(char const *);
void set_led_color(char const *);
void get_led_color(char const *);
void set_led_rate(char const *);
void get_led_rate(char const *);


static std::map<std::string, HandlerFuncPtr> const CMD = {
    { "set-led-state", set_led_state }
  , { "get-led-state", get_led_state }
  , { "set-led-color", set_led_color }
  , { "get-led-color", get_led_color }
  , { "set-led-rate",  set_led_rate }
  , { "get-led-rate",  get_led_rate }
};


void LedServer::server_business(DataBuffer data, LedServer::Client& client)
{
    std::string input(reinterpret_cast<char*>(data.data()));
    size_t com = input.find_first_of(" \n\0");

    if (com == std::string::npos)
    {
        return;
    }

    auto it = CMD.find(input.substr(0, com));
    if (it != CMD.end())
    {
        (*it).second(input.c_str());
    }
    else
    {
        // TODO: make a warning log
    }

    //client.sendData("Hello, client!\0", sizeof("Hello, client!\0"));
};


void set_led_state(char const *args)
{
}


void get_led_state(char const *args)
{
}


void set_led_color(char const *args)
{
}


void get_led_color(char const *args)
{
}


void set_led_rate(char const *args)
{
}


void get_led_rate(char const *args)
{
}
