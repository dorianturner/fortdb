CC := gcc
CFLAGS := -D_POSIX_C_SOURCE=200809L -Wall -Wextra -std=c11 -I../src -I../src/utils

.PHONY: all clean

all: test_hash test_version_node

test_hash:
	$(CC) $(CFLAGS) test_hash.c ../src/utils/hash.c ../src/utils/version_node.c -o test_hash

test_version_node:
	$(CC) $(CFLAGS) test_version_node.c ../src/utils/version_node.c -o test_version_node

clean:
	rm -f test_hash test_version_node *.o

