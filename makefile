# enables debug symbols, CPU trace logging
DEBUG = 0

# enables verbose cmake for diagnosing the make step, and the cmake build command lines (1 = build steps, 2 = cmake trace)
VERBOSE_CMAKE = 1

SHORTHASH = "$(shell git rev-parse --short HEAD || unknown)"

# static libraries
ZLIB_INCLUDE = $(PWD)/zlib_build/include
SDL2_INCLUDE = $(PWD)/SDL/build/include
ZLIB_LIB = $(PWD)/zlib_build/lib/libz.a
SDL2_LIB = $(PWD)/SDL/build/lib/libSLD2.a
SDL2_LINK = $(shell $(PWD)/SDL/build/bin/sdl2-config --static-libs)
ZLIB_LINK = $(ZLIB_LIB)

CC ?= gcc
CFLAGS += \
	-O2 -Wall -Werror -fPIC \
	-D__LIBRETRO__ -DSHORTHASH=\"$(SHORTHASH)\" \
	-Ihatari/build -I$(SDL2_INCLUDE)
LDFLAGS += \
	-shared -Wall -Werror -static-libgcc

CMAKEFLAGS += \
	-DZLIB_INCLUDE_DIR=$(ZLIB_INCLUDE) \
	-DZLIB_LIBRARY=$(ZLIB_LIB) \
	-DSDL2_INCLUDE_DIR=$(SDL2_INCLUDE)/SDL2 \
	-DSDL2_LIBRARY=$(SDL2_LIB) \
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
ifeq ($(VERBOSE_CMAKE),2)
	CMAKEFLAGS += --trace
endif
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
	$(ZLIB_LINK) $(SDL2_LINK)
# note: libcore is linked twice to allow other hatari internal libraries to resolve references within it.

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
