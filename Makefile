
CC := clang
CFLAGS := -m64 -std=c11 -Isrc
DEBUG_CFLAGS := -DDEBUG -g -O0 -fsanitize=address
RELEASE_CFLAGS := -DNDEBUG -O2

CFLAGS += -Wall -Wextra -pedantic -Wconversion
CFLAGS += -Wno-gnu-binary-literal -Wno-c23-extensions

config ?= debug
gfx_api ?= vulkan
 
ifeq ($(config), debug)
	CFLAGS += $(DEBUG_CFLAGS)
else
	CFLAGS += $(RELEASE_CFLAGS)
endif

ifeq ($(gfx_api), vulkan)
	CFLAGS += -DWIN_GFX_API_VULKAN
	CFLAGS += -I$(VULKAN_SDK)/Include/
else ifeq ($(gfx_api), opengl)
	CFLAGS += -DWIN_GFX_API_OPENGL
endif

# OS-Specific Stuff
LFLAGS =
MKDIR_BIN = 
RM_BIN = 
BIN_EXT = 

ifeq ($(OS), Windows_NT)
	LFLAGS += -lgdi32 -lkernel32 -luser32 -lBcrypt -lshcore -lhid

	ifeq ($(gfx_api), vulkan)
		LFLAGS += -L$(VULKAN_SDK)\Lib -lvulkan-1
	else ifeq ($(gfx_api), opengl)
		LFLAGS += -lopengl32
	endif

	MKDIR_BIN = if not exist bin\$(config) mkdir bin\$(config)
	MKDIR_BIN += && mkdir bin\$(config)\spirv
	RM_BIN = rd /s /q bin
	BIN_EXT = .exe
else
	# TODO: vulkan stuff for Linux
	LFLAGS += -lm -lX11 -lGL -lGLX
	MKDIR_BIN = mkdir -p bin/$(config)
	MKDIR_BIN += && mkdir -p bin/$(config)/spirv
	RM_BIN = rm -r bin
endif

SRC_DIR := src
BIN := bin/$(config)/Octopus

SLANGC := slangc

SLANG_FLAGS := -target spirv -profile spirv_1_4 -emit-spirv-directly
SLANG_FLAGS += -fvk-use-entrypoint-name

SHADER_SRC_DIR := $(SRC_DIR)/shaders
SHADER_IR_DIR := bin/$(config)/spirv

SHADER_SRCS := $(wildcard $(SHADER_SRC_DIR)/*.slang)
SHADER_IRS := $(patsubst %.slang,$(SHADER_IR_DIR)/%.spv,$(notdir $(SHADER_SRCS)))

all: Octopus

$(SHADER_IR_DIR)/%.spv: $(SHADER_SRC_DIR)/%.slang
	@$(MKDIR_BIN)
	$(SLANGC) $(SLANG_FLAGS) $< -o $@

Octopus: $(SHADER_IRS)
	@$(MKDIR_BIN)
	$(CC) $(SRC_DIR)/main.c $(CFLAGS) $(LFLAGS) -o $(BIN)$(BIN_EXT)

clean:
	$(RM_BIN)

.PHONY: all Octopus clean

