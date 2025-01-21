CC = clang
CFLAGS = -m64 -std=c99 -Isrc
DEBUG_FLAGS = -DDEBUG -g -O0
RELEASE_FLAGS = -DNDEBUG -O2

CFLAGS += -Weverything -Wno-error=padded -Wno-declaration-after-statement

config ?= debug
 
ifeq ($(config), debug)
	CFLAGS += $(DEBUG_CFLAGS)
else
	CFLAGS += $(RELEASE_CFLAGS)
endif

# TODO: add windows link flags
LFLAGS = -lm -lX11 -lGL -lGLX

SRC_DIR = src
BIN = bin/$(config)/Octopus

all: Octopus

Octopus:
	@mkdir -p bin
	@mkdir -p bin/$(config)
	$(CC) $(SRC_DIR)/main.c $(CFLAGS) $(LFLAGS) -o $(BIN)

clean:
	rm -r bin

.PHONY: all Octopus clean

