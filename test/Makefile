CC := gcc
CFLAGS := -D_POSIX_C_SOURCE=200809L -Wall -Wextra -std=c11 -I../src -I../src/utils
SRCS := test_hash.c ../src/utils/hash.c ../src/utils/version_node.c
TARGET := test_hash

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(SRCS)
	$(CC) $(CFLAGS) $(filter %.c,$^) -o $(TARGET)

clean:
	rm -f $(TARGET) *.o

