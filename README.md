# HInit

**HInit** is a lightweight Unix-style init system written in **C**.

It is designed around a simple principle:

> Keep PID 1 small, explicit, predictable, and composed of independent components.

HInit is being developed as the init system for **Hail2**, but is designed to remain independent enough to be used and tested on other Unix-like systems.

---

## Status

🚧 **Early development**

HInit is currently functional enough to boot inside an isolated test environment and execute its system preparation and shutdown paths.

The project is still under active development and should **not** yet be used as the real PID 1 of a production system.

---

## Design

HInit is divided into several independent parts instead of putting the entire init system into one large program.

```text
                    ┌──────────────┐
                    │    HInit     │
                    │    PID 1     │
                    └──────┬───────┘
                           │
        ┌──────────────────┼──────────────────┐
        │                  │                  │
        ▼                  ▼                  ▼
    SYSLOAD             Modes              Signals
        │                  │
        │            ┌─────┼─────┐
        │            │     │     │
        │           MD0   MD1   MD2
        │
        ▼
   System setup

        │
        ▼
       HSV
        │
        ├── service 1
        ├── service 2
        ├── service 3
        └── ...
```

The service system is inspired by the simplicity of runit, while HInit uses its own architecture and terminology.

---

# Machine Modes

HInit uses three machine modes:

| Mode  | Name      | Purpose                                       |
| ----- | --------- | --------------------------------------------- |
| `MD0` | Shutdown  | Stop services and shut the machine down       |
| `MD1` | Startup   | Prepare the system and start enabled services |
| `MD2` | Operating | Normal system operation                       |

### MD1

The startup mode is responsible for bringing the system into an operational state.

The startup path includes:

```text
SYSLOAD
   │
   ├── Virtual filesystems
   ├── Device filesystem
   ├── Runtime environment
   ├── Filesystem checks
   ├── Filesystem mounts
   └── Swap
          │
          ▼
        MD1
          │
          ▼
      Services
```

### MD2

`MD2` represents the normal operating state of the machine.

This is where the system remains after startup is complete.

### MD0

`MD0` is the shutdown state.

The shutdown path is responsible for stopping services and preparing the system for kernel shutdown:

```text
MD0
 │
 ├── Stop HSV instances
 ├── Shutdown hooks
 ├── Disable swap
 ├── Unmount filesystems
 ├── sync()
 └── kernel poweroff
```

---

# SYSLOAD

`SYSLOAD` is the system preparation layer.

Its purpose is to prepare the basic runtime environment before HInit enters the machine's normal modes.

Current preparation phases include:

```text
Virtual filesystem
        ↓
Device filesystem
        ↓
Runtime environment
        ↓
Filesystem checks
        ↓
Filesystem mounts
        ↓
Swap
```

The implementation lives in:

```text
[SYSLOAD]/
├── sysload.c
├── sysload.h
├── sysdown.c
└── sysdown.h
```

---

# SYSDOWN

`SYSDOWN` handles system shutdown preparation.

Its current responsibilities include:

* disabling swap
* unmounting filesystems
* synchronizing filesystem buffers
* requesting kernel poweroff

The shutdown implementation uses the kernel reboot interface:

```c
reboot(RB_POWER_OFF);
```

The intention is to keep shutdown explicit rather than building another large abstraction layer around it.

---

# HSV

**HSV — HInit Service Supervisor**

HSV is the per-service supervisor used by HInit.

The design is intentionally similar to the idea behind runit's `runsv`:

> One supervisor instance belongs to one service.

For example:

```text
HSV nginx
HSV sshd
HSV dbus
HSV cron
```

Each instance is responsible for its own service lifecycle.

A service can contain:

```text
service/
├── run
├── stop
└── finish
```

### `run`

Starts the service.

### `stop`

Requests service shutdown.

### `finish`

Runs after the service terminates.

HSV also supervises the service process and can restart it when appropriate.

---

# Service Loading

Services are enabled through filesystem links rather than through a complicated database.

The startup directory is:

```text
/etc/hinit/[LOAD]/onboot/
```

For example:

```bash
ln -s /etc/hinit/[SV]/nginx \
    /etc/hinit/[LOAD]/onboot/nginx
```

Removing the link disables the service:

```bash
rm /etc/hinit/[LOAD]/onboot/nginx
```

This keeps **service administration** separate from **service enablement**.

HInit does not need a database to know which services are enabled.

The filesystem itself describes the configuration.

---

# Service Directory

Services live under:

```text
/etc/hinit/[SV]/
```

Example:

```text
/etc/hinit/[SV]/
└── nginx/
    ├── run
    ├── stop
    └── finish
```

Enabled services are represented by links in:

```text
/etc/hinit/[LOAD]/onboot/
```

---

# Planned Control Interface

HInit is planned to expose a Unix domain socket for communication with PID 1:

```text
/run/hinit/feed.sock
```

The communication layer will be handled by **HInit Feed**.

The protocol is intended to remain lightweight and human-readable.

Example commands:

```text
[MD0]
[MD1]
[MD2]

[POWEROFF]
[REBOOT]
```

Service administration will remain separate from PID 1 control.

---

# HInit Feed

`hinitfeed` will communicate directly with the HInit PID 1 process.

Its responsibility is machine-level control.

Examples:

```bash
hinitfeed [MD0]
hinitfeed [MD1]
hinitfeed [MD2]

hinitfeed [POWEROFF]
hinitfeed [REBOOT]
```

It will **not** be responsible for directly administering individual services.

---

# HSV Control

Service administration will be handled separately by:

```text
hsvctl
```

`hsvctl` will communicate with the HSV instances and provide operations such as:

```text
start
stop
restart
status
```

The distinction is intentional:

```text
hinitfeed
    │
    └── HInit / machine state

hsvctl
    │
    └── HSV / services
```

Service enablement remains filesystem-based.

---

# Project Structure

Current project structure:

```text
hinit/
├── [ENG]/
│   ├── hinit.c
│   ├── hinit.h
│   ├── hsv.c
│   ├── load.c
│   ├── mode.c
│   ├── process.c
│   ├── process.h
│   └── signal.c
│
├── [LOAD]/
│   ├── onboot/
│   └── onshut/
│
├── [MD]/
│   ├── md0/
│   ├── md1/
│   └── md2/
│
├── [SV]/
│
├── [SYSLOAD]/
│   ├── sysload.c
│   ├── sysload.h
│   ├── sysdown.c
│   └── sysdown.h
│
└── Makefile
```

The bracketed directory names are intentional and are part of the project's organization.

---

# Building

HInit currently uses GCC for development.

Build the project with:

```bash
make
```

The resulting binaries are placed in:

```text
[ENG]/build/
```

The main binaries are:

```text
[ENG]/build/hinit
[ENG]/build/hsv
```

---

# Testing

HInit can currently be tested inside an isolated root filesystem using Linux namespaces.

The project Makefile provides:

```bash
make test
```

The test environment uses:

```text
mount namespace
PID namespace
/proc
isolated root filesystem
```

This allows HInit to run as PID 1 without replacing the host system's real init.

---

# Installation

The current installation target installs:

```text
/sbin/hinit
/usr/lib/hinit/hsv
```

and creates the required HInit directories under:

```text
/etc/hinit/
```

Installation:

```bash
sudo make install
```

**Do not use this as your real PID 1 on a production system yet.**

---

# Development Philosophy

HInit intentionally avoids unnecessary complexity.

The project favors:

* C
* POSIX/Linux primitives
* Unix domain sockets
* filesystem-based configuration
* independent processes
* explicit process supervision
* small components
* predictable behavior
* simple service definitions

HInit does **not** aim to reproduce systemd's architecture.

The goal is a Unix-style init where the important parts of the system can be inspected directly and understood without requiring a large abstraction stack.

---

# Roadmap

### Current

* [x] PID 1 prototype
* [x] SYSLOAD
* [x] SYSDOWN
* [x] MD0 / MD1 / MD2
* [x] Process management
* [x] Signal handling
* [x] HSV prototype
* [x] Per-service HSV model
* [x] Filesystem-based service enablement
* [x] Isolated PID 1 testing

### Next

* [ ] HInit Feed
* [ ] Unix domain socket interface
* [ ] `hinitfeed`
* [ ] `hsvctl`
* [ ] Full service lifecycle management
* [ ] Better signal handling
* [ ] Service status reporting
* [ ] Robust child reaping
* [ ] More complete boot/shutdown handling
* [ ] Real hardware boot testing

### Future

* [ ] Production-ready PID 1
* [ ] Hail2 integration
* [ ] Additional service supervision features
* [ ] Documentation
* [ ] Portability improvements

---

# Warning

HInit is experimental software.

Running an init system as PID 1 directly controls the lifecycle of the entire operating system.

Until HInit reaches production readiness, use the isolated test environment or a disposable virtual machine.

---

# License

License: **TBD**

---

# Hail2

HInit is being developed as part of the **Hail2** operating system project.

Hail2 is intended to use a lightweight Unix-style userspace with a focus on explicit system components and minimal unnecessary abstraction.
