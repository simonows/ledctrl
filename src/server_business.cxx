#include <ledctrl/server_base.h>
#include <iostream>
#include <map>

#define OKAY -1
#define BAD -2

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
typedef std::string (*HandlerFuncPtr)(std::string);

typedef struct Led
{
    enum LedState state;
    enum LedColor color;
    LedRate rate;
} Led;

static Led target = { ON, RED, 4 };
static std::mutex cons_mutex;

std::string set_led_state(std::string);
std::string get_led_state(std::string);
std::string set_led_color(std::string);
std::string get_led_color(std::string);
std::string set_led_rate(std::string);
std::string get_led_rate(std::string);

void print_screen(void);


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
    std::string rc;
    std::string input(reinterpret_cast<char*>(data.data()));
    size_t com = input.find_first_of(" \n\0");

    auto it = CMD.find(input.substr(0, com));
    if (it == CMD.end())
    {
        // TODO: make a warning log
        return;
    }

    if (com != std::string::npos)
        rc = (*it).second(input.substr(com + 1));
    else
        rc = (*it).second("");

    client.sendData(rc);
};


std::string set_led_state(std::string args)
{
    std::lock_guard lock(cons_mutex);
    args = args.substr(0, args.find_first_of("\n"));
    if (args == "off") target.state = OFF;
    else if (args == "on") target.state = ON;
    else return "FAILED\n";
    LedServer::print_screen();
    return "OK\n";
}


std::string get_led_state(std::string args)
{
    std::lock_guard lock(cons_mutex);
    switch (target.state)
    {
        case ON:
            return "OK on\n";
        case OFF:
            return "OK off\n";
        default:
            break;
    }
    return "FAILED\n";
}


std::string set_led_color(std::string args)
{
    std::lock_guard lock(cons_mutex);
    args = args.substr(0, args.find_first_of("\n"));
    if (args == "green") target.color = GREEN;
    else if (args == "red") target.color = RED;
    else if (args == "blue") target.color = BLUE;
    else return "FAILED\n";
    LedServer::print_screen();
    return "OK\n";
}


std::string get_led_color(std::string args)
{
    std::lock_guard lock(cons_mutex);
    switch (target.color)
    {
        case RED:
            return "OK red\n";
        case GREEN:
            return "OK green\n";
        case BLUE:
            return "OK blue\n";
        default:
            break;
    }
    return "FAILED\n";
}


std::string set_led_rate(std::string args)
{
    std::lock_guard lock(cons_mutex);
    args = args.substr(0, args.find_first_of("\n"));
    int val = std::atoi(args.c_str());
    if (val >= 0 && val <= 5) target.rate = val;
    else return "FAILED\n";
    LedServer::print_screen();
    return "OK\n";
}


std::string get_led_rate(std::string args)
{
    std::lock_guard lock(cons_mutex);
    char buf[100];
    std::snprintf(buf, sizeof(buf), "OK %u\n", target.rate);
    return std::string(buf);
}

void LedServer::print_screen(void)
{
    system("clear");
    std::cout << "State: ";
    switch (target.state)
    {
        case ON:
            std::cout << "ON";
            break;
        case OFF:
            std::cout << "OFF";
            break;
        default:
            break;
    }
    std::cout << std::endl;
    std::cout << "Color: ";
    switch (target.color)
    {
        case RED:
            std::cout << "RED";
            break;
        case GREEN:
            std::cout << "GREEN";
            break;
        case BLUE:
            std::cout << "BLUE";
            break;
        default:
            break;
    }
    std::cout << std::endl;
    std::cout << "Rate:  " << std::to_string(static_cast<int>(target.rate)) << std::endl;
}
