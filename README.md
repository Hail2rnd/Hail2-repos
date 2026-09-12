# HInit

HInit is the init system being developed for Hail2.

It is written in C and runs as PID 1. The main idea is to keep the init system small, simple and easy to understand.

## Modes

HInit has three machine modes:

* MD0 — shutdown
* MD1 — startup
* MD2 — operating

MD1 and MD0 run their respective tasks and finish. MD2 stays active while the system is running.

## Services

HInit uses a service supervisor called hsv.

Services are stored under `/etc/hinit/[SV]/`.

Boot services are selected through `/etc/hinit/[LOAD]/onboot/`.

The service system is inspired by runit, but HInit is its own implementation.

## Logging

HInit has its own logging system.

Logs are written to `/var/log/logs.log`.

There are four log levels:

* LOG
* WARN
* CRITICAL
* FATAL

The `logger` program provides a terminal interface for viewing the logs.

## Source

The project is divided into a few main parts:

* `[ENG]` — HInit's main engine, process handling and service supervision
* `[FEED]` — HInit control interface
* `[LOG]` — logging system and logger
* `[MD]` — machine modes
* `[SYSLOAD]` — system startup and shutdown preparation

## Building

The Makefile is only used to compile the project. It does not install anything or run tests.

Run `make` to build HInit.

Run `make clean` to remove the build files.

Build files are stored in `[ENG]/build/`.

The compiler can be selected when running Make. Both GCC and Clang can be used.

## Status

HInit is part of Hail2 and is currently under development.

Current version: Alphabuild 1.0
