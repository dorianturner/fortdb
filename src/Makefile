CC         := gcc
PKG_CONFIG := pkg-config
PKG_LIBS   :=

PKG_CFLAGS  := $(if $(PKG_LIBS),$(shell $(PKG_CONFIG) --cflags $(PKG_LIBS)),)
PKG_LDFLAGS := $(if $(PKG_LIBS),$(shell $(PKG_CONFIG) --libs $(PKG_LIBS)),)

# Detect Windows vs POSIX
ifeq ($(OS),Windows_NT)
	CFLAGS   := -DWIN32 -Wall -Wextra -std=c11 $(PKG_CFLAGS) -I. -Iutils
	LDFLAGS  := $(PKG_LDFLAGS)
	RM       := del /Q
else
	CFLAGS   := -D_POSIX_C_SOURCE=200112L -Wall -Wextra -std=c11 -pthread $(PKG_CFLAGS) -I. -Iutils
	LDFLAGS  := -pthread $(PKG_LDFLAGS)
	RM       := rm -f
endif

SRC      := $(wildcard *.c) $(wildcard utils/*.c)
OBJ      := $(SRC:.c=.o)
TARGET   := fortdb

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(OBJ) -o $@ $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	$(RM) $(OBJ) $(TARGET)
