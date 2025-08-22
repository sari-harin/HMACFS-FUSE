# Build for FUSE3
CC      ?= gcc
CFLAGS  ?= -O2 -Wall -Wextra -Wno-unused-parameter -std=c11 -D_FILE_OFFSET_BITS=64
PKGCFG  ?= pkg-config
FUSE_CFLAGS := $(shell $(PKGCFG) --cflags fuse3)
FUSE_LIBS   := $(shell $(PKGCFG) --libs   fuse3)

INCLUDES := -Iinclude
SRC := src/main.c src/fuse_operations.c
BIN := hmacfs

all: $(BIN)

$(BIN): $(SRC)
	$(CC) $(CFLAGS) $(INCLUDES) $(FUSE_CFLAGS) -o $@ $(SRC) $(FUSE_LIBS)

clean:
	rm -f $(BIN)

.PHONY: all clean