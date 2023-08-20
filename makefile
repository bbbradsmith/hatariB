# enables debug symbols, CPU trace logging
DEBUG = 0

# enables verbose cmake for diagnosing the make step, and the cmak build command lines
VERBOSE_CMAKE = 1

SHORTHASH = "$(shell git rev-parse --short HEAD || unknown)"

CC ?= gcc
CFLAGS += -O2 -Wall -Werror -fPIC -D__LIBRETRO__ -DSHORTHASH=\"$(SHORTHASH)\" -Ihatari/build
LDFLAGS += -shared -Wall -Werror -static-libgcc
CMAKEFLAGS += \
	-DCMAKE_DISABLE_FIND_PACKAGE_Readline=1 \
	-DCMAKE_DISABLE_FIND_PACKAGE_X11=1 \
	-DCMAKE_DISABLE_FIND_PACKAGE_PNG=1 \
	-DCMAKE_DISABLE_FIND_PACKAGE_PortMidi=1 \
	-DCMAKE_DISABLE_FIND_PACKAGE_CapsImage=1 \
	-DENABLE_SMALL_MEM=0
CMAKEBUILDFLAGS += -j

ifneq ($(DEBUG),0)
	CFLAGS += -g
	LDFLAGS += -g
	CMAKEFLAGS += -DENABLE_TRACING=1
else
	CMAKEFLAGS += -DENABLE_TRACING=0
endif

ifneq ($(VERBOSE_CMAKE),0)
	CMAKEFLAGS += --trace
	CMAKEBUILDFLAGS += --verbose
endif

ifeq ($(OS),Windows_NT)
	SO_SUFFIX=.dll
else ifeq ($(OS),MacOS)
	SO_SUFFIX=.dylib
else
	SO_SUFFIX=.so
endif

BD=build/
CORE=$(BD)hatarib$(SO_SUFFIX)
SOURCES = \
	core/core.c \
	core/core_file.c \
	core/core_input.c \
	core/core_disk.c \
	core/core_config.c \
	core/core_osk.c
OBJECTS = $(SOURCES:%.c=$(BD)%.o)
HATARILIBS = \
	hatari/build/src/libcore.a \
	hatari/build/src/falcon/libFalcon.a \
	hatari/build/src/cpu/libUaeCpu.a \
	hatari/build/src/gui-sdl/libGuiSdl.a \
	hatari/build/src/libFloppy.a \
	hatari/build/src/debug/libDebug.a \
	hatari/build/src/libcore.a \
	-lz
# note: libcore is linked twice to allow other hatari internal libraries to resolve references within it.

# MacOS has to include SDL2 as a framework instead of a library
ifneq ($(OS),MacOS)
	HATARILIBS += -lSDL2
else
	CFLAGS += -framework SDL2
	LDLAGS += -framework SDL2
endif

default: $(CORE)

directories:
	mkdir -p $(BD)
	mkdir -p $(BD)core
	mkdir -p hatari/build

$(CORE): directories hatarilib $(OBJECTS)
	$(CC) -o $(CORE) $(LDFLAGS) $(OBJECTS) $(HATARILIBS)

$(BD)core/%.o: core/%.c
	$(CC) -o $@ $(CFLAGS) -c $< 

hatarilib: directories
	(cd hatari/build && cmake .. $(CMAKEFLAGS))
	(cd hatari/build && cmake --build . $(CMAKEBUILDFLAGS))

clean:
	rm -f -r $(BD)
	rm -f -r hatari/build
