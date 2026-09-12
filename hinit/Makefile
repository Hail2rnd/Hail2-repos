# ==========================================================
# HInit Build System
# ==========================================================

CC      := clang
CFLAGS  := -Wall -Wextra -O2
LDFLAGS :=

ENG     := [ENG]
LOG     := [LOG]
MD      := [MD]
SYSLOAD := [SYSLOAD]
BUILD   := $(ENG)/build


# ==========================================================
# COMMON OBJECTS
# ==========================================================

COMMON_OBJS := \
	$(BUILD)/log.o \
	$(BUILD)/process.o


# ==========================================================
# HINIT
# ==========================================================

HINIT_OBJS := \
	$(BUILD)/hinit.o \
	$(BUILD)/mode.o \
	$(BUILD)/load.o \
	$(BUILD)/signal.o \
	$(BUILD)/sysload.o \
	$(BUILD)/sysdown.o \
	$(BUILD)/log.o \
	$(BUILD)/process.o


# ==========================================================
# HSV
# ==========================================================

HSV_OBJS := \
	$(BUILD)/hsv.o \
	$(BUILD)/process-hsv.o \
	$(BUILD)/log.o


# ==========================================================
# LOGGER
# ==========================================================

LOGGER_OBJS := \
	$(BUILD)/logger.o \
	$(BUILD)/log.o


# ==========================================================
# MACHINE MODES
# ==========================================================

MD0_OBJS := \
	$(BUILD)/md0.o \
	$(BUILD)/hsv-md0.o \
	$(BUILD)/process-md0.o \
	$(BUILD)/sysdown-md0.o \
	$(BUILD)/log.o


MD1_OBJS := \
	$(BUILD)/md1.o \
	$(BUILD)/load-md1.o \
	$(BUILD)/process-md1.o \
	$(BUILD)/sysload-md1.o \
	$(BUILD)/log.o


MD2_OBJS := \
	$(BUILD)/md2.o \
	$(BUILD)/log.o


# ==========================================================
# TARGETS
# ==========================================================

TARGETS := \
	$(BUILD)/hinit \
	$(BUILD)/hsv \
	$(BUILD)/logger \
	$(BUILD)/md0 \
	$(BUILD)/md1 \
	$(BUILD)/md2


.PHONY: all clean


all: $(TARGETS)


# ==========================================================
# BUILD DIRECTORY
# ==========================================================

$(BUILD):
	mkdir -p $(BUILD)


# ==========================================================
# HINIT OBJECTS
# ==========================================================

$(BUILD)/hinit.o: $(ENG)/hinit.c | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@


$(BUILD)/mode.o: $(ENG)/mode.c | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@


$(BUILD)/load.o: $(ENG)/load.c | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@


$(BUILD)/signal.o: $(ENG)/signal.c | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@


$(BUILD)/process.o: $(ENG)/process.c | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@


$(BUILD)/sysload.o: $(SYSLOAD)/sysload.c | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@


$(BUILD)/sysdown.o: $(SYSLOAD)/sysdown.c | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@


# ==========================================================
# LOG OBJECTS
# ==========================================================

$(BUILD)/log.o: $(LOG)/log.c $(LOG)/log.h | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@


$(BUILD)/logger.o: $(LOG)/logger.c $(LOG)/log.h | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@


# ==========================================================
# HSV OBJECTS
# ==========================================================

$(BUILD)/hsv.o: $(ENG)/hsv.c | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@


$(BUILD)/process-hsv.o: $(ENG)/process.c | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@


# ==========================================================
# MD OBJECTS
# ==========================================================

$(BUILD)/md0.o: $(MD)/md0.c | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@


$(BUILD)/md1.o: $(MD)/md1.c | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@


$(BUILD)/md2.o: $(MD)/md2.c | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@


$(BUILD)/hsv-md0.o: $(ENG)/hsv.c | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@


$(BUILD)/process-md0.o: $(ENG)/process.c | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@


$(BUILD)/sysdown-md0.o: $(SYSLOAD)/sysdown.c | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@


$(BUILD)/load-md1.o: $(ENG)/load.c | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@


$(BUILD)/process-md1.o: $(ENG)/process.c | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@


$(BUILD)/sysload-md1.o: $(SYSLOAD)/sysload.c | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@


# ==========================================================
# LINK
# ==========================================================

$(BUILD)/hinit: $(HINIT_OBJS)
	$(CC) $(HINIT_OBJS) $(LDFLAGS) -o $@


$(BUILD)/hsv: $(HSV_OBJS)
	$(CC) $(HSV_OBJS) $(LDFLAGS) -o $@


$(BUILD)/logger: $(LOGGER_OBJS)
	$(CC) $(LOGGER_OBJS) $(LDFLAGS) -o $@


$(BUILD)/md0: $(MD0_OBJS)
	$(CC) $(MD0_OBJS) $(LDFLAGS) -o $@


$(BUILD)/md1: $(MD1_OBJS)
	$(CC) $(MD1_OBJS) $(LDFLAGS) -o $@


$(BUILD)/md2: $(MD2_OBJS)
	$(CC) $(MD2_OBJS) $(LDFLAGS) -o $@


# ==========================================================
# CLEAN
# ==========================================================

clean:
	rm -rf $(BUILD)
