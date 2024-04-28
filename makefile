# BD = core build directory
# HBD = hatari build subdirectory
BD ?= build
HBD ?= build
ZLIB_BUILD ?= zlib_build

# enables debug symbols, CPU trace logging
DEBUG ?= 0

# enables verbose cmake for diagnosing the make step, and the cmake build command lines (1 = build steps, 2 = cmake trace)
VERBOSE_CMAKE ?= 0

# if multithreaded make causes problems try setting MULTITHREAD to be nothing
MULTITHREAD ?= -j

# to disable warnings as errors try setting WERROR to be nothing
WERROR ?= -Wall -Werror

# git revision hash
SHORTHASH = "$(shell git rev-parse --short HEAD || unknown)"

# static libraries
ZLIB_INCLUDE ?= $(PWD)/$(ZLIB_BUILD)/include
SDL2_INCLUDE ?= $(PWD)/SDL/build/include/SDL2
ZLIB_LIB ?= $(PWD)/$(ZLIB_BUILD)/lib/libz.a
SDL2_LIB ?= $(PWD)/SDL/build/lib/libSDL2.a
SDL2_LINK ?= $(shell $(PWD)/SDL/build/bin/sdl2-config --static-libs)
ZLIB_LINK ?= $(ZLIB_LIB)
# sdl2-config is less than ideal, designed for EXE rather than DLL,
# it adds -lSDLmain etc. but it seems the best way to get the mac dependencies right

CC ?= gcc
CFLAGS += \
	-O3 $(WERROR) -fPIC \
	-D__LIBRETRO__ -DSHORTHASH=\"$(SHORTHASH)\" \
	-Ihatari/$(HBD) -I$(SDL2_INCLUDE)
LDFLAGS += \
	-shared $(WERROR)

CMAKE ?= cmake
CMAKEFLAGS += \
	-DZLIB_INCLUDE_DIR=$(ZLIB_INCLUDE) \
	-DZLIB_LIBRARY=$(ZLIB_LIB) \
	-DSDL2_INCLUDE_DIR=$(SDL2_INCLUDE) \
	-DSDL2_LIBRARY=$(SDL2_LIB) \
	-DCMAKE_DISABLE_FIND_PACKAGE_Readline=1 \
	-DCMAKE_DISABLE_FIND_PACKAGE_X11=1 \
	-DCMAKE_DISABLE_FIND_PACKAGE_PNG=1 \
	-DCMAKE_DISABLE_FIND_PACKAGE_PortMidi=1 \
	-DCMAKE_DISABLE_FIND_PACKAGE_CapsImage=1 \
	-DENABLE_SMALL_MEM=0
CMAKEBUILDFLAGS += $(MULTITHREAD)

ifeq ($(DEBUG),1)
	CFLAGS += -g -DCORE_DEBUG=1
	LDFLAGS += -g
	CMAKEFLAGS += -DENABLE_TRACING=1
else
	ifneq ($(shell uname),Darwin)
		LDFLAGS += -Wl,--strip-debug
	endif
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
	LDFLAGS += -static-libgcc
else ifeq ($(shell uname),Darwin)
	SO_SUFFIX=.dylib
else
	SO_SUFFIX=.so
	LDFLAGS += -static-libgcc
endif

CORE=$(BD)/hatarib$(SO_SUFFIX)
SOURCES = \
	core/core.c \
	core/core_file.c \
	core/core_input.c \
	core/core_disk.c \
	core/core_config.c \
	core/core_osk.c
OBJECTS = $(SOURCES:%.c=$(BD)/%.o)
HATARILIBS = \
	hatari/$(HBD)/src/libcore.a \
	hatari/$(HBD)/src/falcon/libFalcon.a \
	hatari/$(HBD)/src/cpu/libUaeCpu.a \
	hatari/$(HBD)/src/gui-sdl/libGuiSdl.a \
	hatari/$(HBD)/src/libFloppy.a \
	hatari/$(HBD)/src/debug/libDebug.a \
	hatari/$(HBD)/src/libcore.a \
	$(ZLIB_LINK) $(SDL2_LINK)
# note: libcore is linked twice to allow other hatari internal libraries to resolve references within it.

.PHONY: default core

default: core

core: $(CORE)

# clean and rebuild everything (including static libs)
full:
	$(MAKE) -f makefile.zlib clean
	$(MAKE) -f makefile.sdl clean
	$(MAKE) clean
	$(MAKE) -f makefile.zlib
	$(MAKE) -f makefile.sdl MULTITHREAD=$(MULTITHREAD)
	$(MAKE) default

sdl:
	$(MAKE) -f makefile.sdl MULTITHREAD=$(MULTITHREAD)

zlib:
	$(MAKE) -f makefile.zlib

# to test a reconfiguration of SDL only
sdlreconfig:
	$(MAKE) -f makefile.sdl clean
	$(MAKE) clean
	$(MAKE) -f makefile.sdl MULTITHREAD=$(MULTITHREAD)
	$(MAKE) default

directories:
	mkdir -p $(BD)
	mkdir -p $(BD)/core
	mkdir -p hatari/$(HBD)

$(CORE): directories hatarilib $(OBJECTS)
	$(CC) -o $(CORE) $(LDFLAGS) $(OBJECTS) $(HATARILIBS)

$(BD)/core/%.o: core/%.c hatarilib
	$(CC) -o $@ $(CFLAGS) -c $< 

hatarilib: directories
	(cd hatari/$(HBD) && export CFLAGS="$(CFLAGS)" && $(CMAKE) .. $(CMAKEFLAGS))
	(cd hatari/$(HBD) && export CFLAGS="$(CFLAGS)" && $(CMAKE) --build . $(CMAKEBUILDFLAGS))

clean:
	rm -f -r $(BD)
	rm -f -r hatari/$(HBD)
