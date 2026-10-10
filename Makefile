CC       ?= cc
CFLAGS   ?= -std=c11 -Wall -Wextra -pedantic -g
CPPFLAGS += -Iinclude

BUILD_DIR := build
APP       := $(BUILD_DIR)/kv-store

APP_SRCS  := src/main.c
LIB_SRCS  := $(filter-out $(APP_SRCS), $(wildcard src/*.c))
TEST_SRCS := $(wildcard tests/*.c)
TEST_BINS := $(patsubst tests/%.c,$(BUILD_DIR)/%,$(TEST_SRCS))
HEADERS   := $(wildcard include/*.h)

.PHONY: all run test clean

all: $(APP)

$(APP): $(APP_SRCS) $(LIB_SRCS) $(HEADERS)
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) $(CPPFLAGS) $(APP_SRCS) $(LIB_SRCS) -o $@

$(BUILD_DIR)/%: tests/%.c $(LIB_SRCS) $(HEADERS)
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) $(CPPFLAGS) $< $(LIB_SRCS) -o $@

run: $(APP)
	$(APP)

test: $(TEST_BINS)
	@status=0; for t in $(TEST_BINS); do \
		echo "--- $$t ---"; \
		"$$t" || status=1; \
	done; \
	exit $$status

clean:
	rm -rf $(BUILD_DIR)