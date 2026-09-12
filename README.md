# HInit

HInit is the init system being developed for Hail2.

The goal is to keep the system simple, predictable, and easy to understand. HInit is written in C and is designed around a small core instead of hiding system behavior behind a large framework.

## What it does

HInit runs as PID 1 and is responsible for the basic system lifecycle.

The current machine states are:

* **MD0** — shutdown
* **MD1** — startup
* **MD2** — operating

MD1 and MD0 are one-shot states. They perform their work and return.

MD2 is the normal operating state and remains active while the system is running.

## Services

HInit uses a service layout inspired by runit.

Services live under:

```text
/etc/hinit/[SV]/
```

A service can have scripts such as:

```text
run
stop
finish
```

The service loader is responsible for deciding which services should be started during boot.

Enabled boot services are represented through:

```text
/etc/hinit/[LOAD]/onboot/
```

The service supervisor (`hsv`) is responsible for supervising individual services and restarting them when necessary.

HInit itself remains responsible for the machine state. Service supervision is kept separate from the machine-state logic.

## System startup

Startup is handled by **MD1**.

The general flow is:

```text
HInit
  |
  v
MD1
  |
  +-- system preparation
  |
  +-- load onboot services
  |
  v
MD2
```

System preparation is kept outside the main engine in `[SYSLOAD]`.

## Shutdown

Shutdown is handled by **MD0**.

The general flow is:

```text
MD0
  |
  +-- stop supervised services
  |
  +-- system shutdown preparation
  |
  v
shutdown
```

There is no `onshut` service directory. Shutdown handling belongs to MD0 and SYSDOWN.

## Logging

HInit has its own small logging system.

Components send messages through:

```c
log_write(
    "HINIT",
    "STARTING PID 1",
    LOG_LEVEL_LOG
);
```

The available levels are:

```text
LOG
WARN
CRITICAL
FATAL
```

Logs are written to:

```text
/var/log/logs.log
```

The log format is intentionally simple:

```text
[09/11/2026] [23:14] MD2 SYSTEM OPERATING LOG
[09/11/2026] [23:15] SYSLOAD FSCK FAILED WARN
```

The `logger` program provides a terminal interface for viewing the log file.

## Source tree

The project is organized into a few main parts:

```text
hinit/
├── Makefile
│
├── [ENG]/
│   ├── hinit.c
│   ├── hinit.h
│   ├── hsv.c
│   ├── load.c
│   ├── mode.c
│   ├── process.c
│   └── signal.c
│
├── [FEED]/
│   ├── feed.c
│   └── feed.h
│
├── [LOG]/
│   ├── log.c
│   ├── log.h
│   └── logger.c
│
├── [MD]/
│   ├── md0.c
│   ├── md1.c
│   └── md2.c
│
└── [SYSLOAD]/
    ├── sysload.c
    ├── sysload.h
    ├── sysdown.c
    └── sysdown.h
```

`[ENG]` contains the main HInit engine and service supervisor.

`[MD]` contains the machine-state implementations.

`[SYSLOAD]` contains system preparation and shutdown preparation.

`[LOG]` contains the logging system and the log viewer.

`[FEED]` provides the control interface for HInit.

## Building

The Makefile is intentionally only a build system.

It does not install anything, create a fake root, or run tests.

Build everything with:

```bash
make
```

Clean the build directory with:

```bash
make clean
```

Build artifacts are kept in:

```text
[ENG]/build/
```

The compiler can be selected when invoking Make. For example:

```bash
make CC=gcc
```

or:

```bash
make CC=clang
```

## Current status

HInit is still under development.

The basic machine-state architecture, service supervision, system-load separation, and logging system are being built before the project is considered ready for actual Hail2 boot integration.

The project is intentionally being built piece by piece instead of trying to implement the whole init system at once.
