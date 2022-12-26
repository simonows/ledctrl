# LedServer
Simple Multi-threading TCP/IP Server

## Сборка

Используется система сборки cmake.
Поддерживаются сборки:
* Make (Linux, Mac)

```zsh
mkdir build && cd build && cmake .. && make
```

## Запуск

```zsh
./src/server

# В другом pty
./src/client
```

## Структура

* [server_business.cxx](src/server_business.cxx) - Реализация ТЗ
* [client_base.cxx](src/client_base.cxx) - Реализации базовой работы клиента по tcp/ip
* [server_base.cxx](src/server_base.cxx) - Реализации базовой работы сервера по tcp/ip
* [main.cxx](src/client/main.cxx) - Клиент
* [main.cxx](src/server/main.cxx) - Сервер

