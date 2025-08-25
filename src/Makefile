# Makefile for fortdb
CC=gcc
CFLAGS=-g -Wall -Wextra -std=c11 -D_POSIX_C_SOURCE=200809L
CPPFLAGS=-I./src -I./src/utils -I./src/storage
LDFLAGS=-pthread

# Path to CLI file with main()
CLI_MAIN ?= src/fortdb.c

UTIL_SRCS := $(wildcard src/utils/*.c)
STOR_SRCS := $(wildcard src/storage/*.c)
CORE_SRCS := $(filter-out $(CLI_MAIN),$(wildcard src/*.c))
APP_SRCS  := $(CLI_MAIN) $(UTIL_SRCS) $(STOR_SRCS) $(CORE_SRCS)
OBJS      := $(APP_SRCS:.c=.o)
TARGET    := fortdb

.PHONY: all clean test

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) $(CPPFLAGS) -c $< -o $@

# Test binary from known-good command
TEST_SRCS := test/test_serialize.c \
             src/storage/serializer.c \
             src/utils/document.c \
             src/utils/version_node.c \
             src/utils/hash.c
TEST_OBJS := $(TEST_SRCS:.c=.o)

test_serialize: $(TEST_OBJS)
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

clean:
	rm -f $(TARGET) test_serialize $(OBJS) $(TEST_OBJS)
