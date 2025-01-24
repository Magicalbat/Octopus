CC = clang
CFLAGS = -m64 -std=c11 -Isrc
DEBUG_FLAGS = -DDEBUG -g -O0 -fsanitize=address
RELEASE_FLAGS = -DNDEBUG -O2

CFLAGS += -Wall -Wextra -pedantic -Wconversion -Wno-gnu-binary-literal

config ?= debug
 
ifeq ($(config), debug)
	CFLAGS += $(DEBUG_CFLAGS)
else
	CFLAGS += $(RELEASE_CFLAGS)
endif

# OS-Specific Stuff
LFLAGS =
MKDIR_BIN = 
RM_BIN = 
BIN_EXT = 

ifeq ($(OS), Windows_NT)
	LFLAGS += -lgdi32 -lkernel32 -luser32 -lBcrypt -lopengl32
	MKDIR_BIN = if not exist bin\$(config) mkdir bin\$(config)
	RM_BIN = rd /s /q bin
	BIN_EXT = .exe
else
	LFLAGS += -lm -lX11 -lGL -lGLX
	MKDIR_BIN = mkdir -p bin/$(config)
	RM_BIN = rm -r bin
endif

SRC_DIR = src
BIN = bin/$(config)/Octopus

all: Octopus

Octopus:
	@$(MKDIR_BIN)
	$(CC) $(SRC_DIR)/main.c $(CFLAGS) $(LFLAGS) -o $(BIN)$(BIN_EXT)

clean:
	$(RM_BIN)

.PHONY: all Octopus clean

