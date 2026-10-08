CC       ?= cc
CFLAGS   ?= -std=c11 -Wall -Wextra -pedantic -g
CPPFLAGS += -Iinclude

BUILD_DIR := build
APP       := $(BUILD_DIR)/kv-store
TEST_BIN  := $(BUILD_DIR)/test_kv

APP_SRCS  := src/main.c
LIB_SRCS  := $(filter-out $(APP_SRCS), $(wildcard src/*.c))
TEST_SRCS := $(wildcard tests/*.c)
HEADERS   := $(wildcard include/*.h)

.PHONY: all run test clean

all: $(APP)

$(APP): $(APP_SRCS) $(LIB_SRCS) $(HEADERS)
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) $(CPPFLAGS) $(APP_SRCS) $(LIB_SRCS) -o $@

$(TEST_BIN): $(TEST_SRCS) $(LIB_SRCS) $(HEADERS)
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) $(CPPFLAGS) $(TEST_SRCS) $(LIB_SRCS) -o $@

run: $(APP)
	$(APP)

test: $(TEST_BIN)
	$(TEST_BIN)

clean:
	rm -rf $(BUILD_DIR)