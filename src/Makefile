CC = gcc
CFLAGS = -Wall -Wextra -std=c17 -I./src -I./src/utils -pthread -D_POSIX_C_SOURCE=200112L
LDFLAGS = -pthread
SRC = $(wildcard src/*.c src/utils/*.c)
OBJ = $(SRC:.c=.o)
EXE = fortdb_test

.PHONY: check-syntax clean syntax-check link-check

check-syntax: syntax-check link-check

syntax-check:
	@echo "Checking syntax for all source files..."
	@for file in $(SRC); do \
		echo "Checking $$file..."; \
		$(CC) $(CFLAGS) -fsyntax-only $$file || exit 1; \
	done

link-check:
	@echo "Checking linking all files..."
	$(CC) $(CFLAGS) $(LDFLAGS) -o $(EXE) $(SRC)
	@echo "Linking OK"

clean:
	rm -f $(EXE) $(OBJ)

