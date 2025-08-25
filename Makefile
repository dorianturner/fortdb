CC = gcc
CFLAGS = -g -Wall -Wextra -std=c11 -D_POSIX_C_SOURCE=200809L -pthread
INCLUDES = -I./src -I./src/utils -I./src/storage
SRC_DIR = src
UTILS_DIR = src/utils
STORAGE_DIR = src/storage

# Sources
SRCS = \
	$(SRC_DIR)/fortdb.c \
	$(SRC_DIR)/decode_and_execute.c \
	$(SRC_DIR)/parser.c \
	$(STORAGE_DIR)/compactor.c \
	$(STORAGE_DIR)/serializer.c \
	$(STORAGE_DIR)/deserializer.c \
	$(UTILS_DIR)/document.c \
	$(UTILS_DIR)/version_node.c \
	$(UTILS_DIR)/hash.c \
	$(UTILS_DIR)/visualiser.c

# Objects
OBJS = $(SRCS:.c=.o)

# Target
TARGET = fortdb.out

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o $@

%.o: %.c
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)

.PHONY: all clean
