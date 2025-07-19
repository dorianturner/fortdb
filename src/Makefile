CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -I. -Iutils -pthread
LDFLAGS = -pthread
SRC = fortdb.c parser.c decode_and_execute.c \
      utils/document.c utils/collection.c  utils/field.c \
      utils/hash.c utils/table.c utils/version_node.c
OBJ = $(SRC:.c=.o)
TARGET = fortdb

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(OBJ) -o $@ $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ) $(TARGET)

