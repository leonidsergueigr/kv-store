CC       ?= cc
CFLAGS   ?= -std=c11 -Wall -Wextra -pedantic -g
CPPFLAGS += -Iinclude

BUILD_DIR := build
APP       := $(BUILD_DIR)/kv-store
TEST_BIN  := $(BUILD_DIR)/test_kv

LIB_SRCS  := src/kv.c
APP_SRCS  := src/main.c
TEST_SRCS := tests/test_kv.c
HEADERS   := include/kv.h

.PHONY: all test clean

all: $(APP)

$(APP): $(APP_SRCS) $(LIB_SRCS) $(HEADERS)
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) $(CPPFLAGS) $(APP_SRCS) $(LIB_SRCS) -o $@

$(TEST_BIN): $(TEST_SRCS) $(LIB_SRCS) $(HEADERS)
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) $(CPPFLAGS) $(TEST_SRCS) $(LIB_SRCS) -o $@

test: $(TEST_BIN)
	$(TEST_BIN)

clean:
	rm -rf $(BUILD_DIR)