# LedServer
Простой многопоточный TCP/IP Server

## Сборка

Используется система сборки cmake.

```zsh
mkdir build && cd build && cmake .. && make
```

## Запуск

```zsh
./build/src/server

# В другом pty
./build/src/client
```

## Структура

* [thread_pool.h](src/include/thread_pool.h) - Пул потоков
* [client_base.cxx](src/client_base.cxx) - Реализация клиента
* [server_base.cxx](src/server_base.cxx) - Реализации сервера
* [main.cxx](src/client/main.cxx) - Тест клиента
* [main.cxx](src/server/main.cxx) - Тест сервера
* [business.cxx](src/business.cxx) - Тестовая логика сервера

