# test/Makefile - builds tests into test/compiled/
CC := gcc
CFLAGS := -D_POSIX_C_SOURCE=200809L -Wall -Wextra -std=c11 -I../src -I../src/utils
LDFLAGS := -pthread

BIN_DIR := compiled

.PHONY: test_compactor test_roundtrip test_serialize test_deserialize

test_compactor: $(BIN_DIR)/test_compactor
test_roundtrip: $(BIN_DIR)/test_roundtrip
test_serialize: $(BIN_DIR)/test_serialize
test_deserialize: $(BIN_DIR)/test_deserialize

# Common utility sources used by most tests
COMMON_SRCS := ../src/utils/hash.c ../src/utils/version_node.c ../src/utils/document.c

# Storage sources (use per-test as needed)
STORAGE_SERIALIZER := ../src/storage/serializer.c ../src/storage/deserializer.c
STORAGE_COMPACTOR  := ../src/storage/compactor.c

# Discover test sources in this dir
TEST_SRCS := $(wildcard test_*.c)
TEST_BINS := $(patsubst %.c,$(BIN_DIR)/%,$(TEST_SRCS))

.PHONY: all clean $(TEST_BINS)

all: $(BIN_DIR) $(TEST_BINS)

# Ensure output dir exists
$(BIN_DIR):
	mkdir -p $(BIN_DIR)

#
# Generic rule: build tests that only need COMMON_SRCS
# If a test needs storage/*.c, add an explicit rule below.
#
$(BIN_DIR)/%: %.c $(COMMON_SRCS) | $(BIN_DIR)
	$(CC) $(CFLAGS) $< $(COMMON_SRCS) $(LDFLAGS) -o $@

#
# Explicit rules for tests that require storage sources.
# Add or edit these if you add/remove tests or source file dependencies.
#

$(BIN_DIR)/test_serializer: test_serializer.c $(COMMON_SRCS) $(STORAGE_SERIALIZER) | $(BIN_DIR)
	$(CC) $(CFLAGS) $< $(COMMON_SRCS) $(STORAGE_SERIALIZER) $(LDFLAGS) -o $@

$(BIN_DIR)/test_roundtrip: test_roundtrip.c $(COMMON_SRCS) $(STORAGE_SERIALIZER) | $(BIN_DIR)
	$(CC) $(CFLAGS) $< $(COMMON_SRCS) $(STORAGE_SERIALIZER) $(LDFLAGS) -o $@

$(BIN_DIR)/test_serialize: test_serialize.c $(COMMON_SRCS) $(STORAGE_SERIALIZER) | $(BIN_DIR)
	$(CC) $(CFLAGS) $< $(COMMON_SRCS) $(STORAGE_SERIALIZER) $(LDFLAGS) -o $@

$(BIN_DIR)/test_deserialize: test_deserialize.c $(COMMON_SRCS) $(STORAGE_SERIALIZER) | $(BIN_DIR)
	$(CC) $(CFLAGS) $< $(COMMON_SRCS) $(STORAGE_SERIALIZER) $(LDFLAGS) -o $@

$(BIN_DIR)/test_compactor: test_compactor.c $(COMMON_SRCS) $(STORAGE_COMPACTOR) | $(BIN_DIR)
	$(CC) $(CFLAGS) $< $(COMMON_SRCS) $(STORAGE_COMPACTOR) $(LDFLAGS) -o $@

#
# Clean
#
clean:
	rm -rf $(BIN_DIR)/*

