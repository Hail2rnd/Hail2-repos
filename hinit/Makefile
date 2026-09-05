# ==========================================================
# HInit Build System
# ==========================================================
TESTROOT := $(HOME)/hinit-test-root
CC      := gcc
CFLAGS  := -Wall -Wextra -O2
LDFLAGS :=

ENG      := [ENG]
SYSLOAD  := [SYSLOAD]
BUILD    := $(ENG)/build

HINIT_SRCS := \
	$(ENG)/hinit.c \
	$(ENG)/mode.c \
	$(ENG)/load.c \
	$(ENG)/signal.c \
	$(ENG)/process.c \
	$(SYSLOAD)/sysload.c \
	$(SYSLOAD)/sysdown.c

HINIT_OBJS := \
	$(BUILD)/hinit.o \
	$(BUILD)/mode.o \
	$(BUILD)/load.o \
	$(BUILD)/signal.o \
	$(BUILD)/process.o \
	$(BUILD)/sysload.o \
	$(BUILD)/sysdown.o

HSV_SRCS := \
	$(ENG)/hsv.c \
	$(ENG)/process.c

HSV_OBJS := \
	$(BUILD)/hsv.o \
	$(BUILD)/process-hsv.o

.PHONY: all clean install uninstall fakeroot test run rebuild

all: $(BUILD) $(BUILD)/hinit $(BUILD)/hsv

$(BUILD):
	mkdir -p $(BUILD)

# ---------- HINIT ----------

$(BUILD)/hinit.o: $(ENG)/hinit.c
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD)/mode.o: $(ENG)/mode.c
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD)/load.o: $(ENG)/load.c
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD)/signal.o: $(ENG)/signal.c
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD)/process.o: $(ENG)/process.c
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD)/sysload.o: $(SYSLOAD)/sysload.c
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD)/sysdown.o: $(SYSLOAD)/sysdown.c
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD)/hinit: $(HINIT_OBJS)
	$(CC) $(HINIT_OBJS) $(LDFLAGS) -o $@

# ---------- HSV ----------

$(BUILD)/hsv.o: $(ENG)/hsv.c
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD)/process-hsv.o: $(ENG)/process.c
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD)/hsv: $(HSV_OBJS)
	$(CC) $(HSV_OBJS) $(LDFLAGS) -o $@

# ---------- CLEAN ----------

clean:
	rm -rf $(BUILD)

# ---------- INSTALL ----------

install:
	install -Dm755 $(BUILD)/hinit /sbin/hinit
	install -Dm755 $(BUILD)/hsv /usr/lib/hinit/hsv

	mkdir -p /etc/hinit/[LOAD]/onboot
	mkdir -p /etc/hinit/[LOAD]/onshut

	mkdir -p /etc/hinit/[SV]

	mkdir -p /etc/hinit/[MD]/md0
	mkdir -p /etc/hinit/[MD]/md1
	mkdir -p /etc/hinit/[MD]/md2

# ---------- FAKEROOT  ---------

fakeroot: all
	@echo "==> Copiando HInit para o ambiente de testes"

	install -Dm755 $(BUILD)/hinit $(TESTROOT)/sbin/hinit
	install -Dm755 $(BUILD)/hsv $(TESTROOT)/usr/lib/hinit/hsv

	mkdir -p $(TESTROOT)/etc/hinit/[LOAD]/onboot
	mkdir -p $(TESTROOT)/etc/hinit/[LOAD]/onshut
	mkdir -p $(TESTROOT)/etc/hinit/[SV]
	mkdir -p $(TESTROOT)/etc/hinit/[MD]/md0
	mkdir -p $(TESTROOT)/etc/hinit/[MD]/md1
	mkdir -p $(TESTROOT)/etc/hinit/[MD]/md2

	@echo "==> Fakeroot atualizado."

# ----------   TEST    ----------

test: fakeroot
	@echo "==> Iniciando HInit"

	sudo unshare \
		--mount \
		--pid \
		--fork \
		--mount-proc \
		--root=$(TESTROOT) \
		/sbin/hinit

run: all fakeroot test
# ---------- UNINSTALL ----------

uninstall:
	rm -f /sbin/hinit
	rm -f /usr/lib/hinit/hsv
