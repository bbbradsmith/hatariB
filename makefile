CC=gcc
CFLAGS=-g -O2 -Wall -Werror -D__LIBRETRO__ -Ihatari/build
LDFLAGS=-shared -g -Wall -Werror
CMAKEFLAGS=-DENABLE_SMALL_MEM=0 -DENABLE_TRACING=0
# for tracing debug:
#CMAKEFLAGS=-DENABLE_SMALL_MEM=0 -DENABLE_TRACING=1

BD=build/
CORE=$(BD)hatarib.dll
SOURCES = \
	core/core.c \
	core/core_file.c \
	core/core_input.c \
	core/core_disk.c \
	core/core_config.c
OBJECTS = $(SOURCES:%.c=$(BD)%.o)
HATARILIBS= \
	hatari/build/src/libcore.a \
	hatari/build/src/falcon/libFalcon.a \
	hatari/build/src/cpu/libUaeCpu.a \
	hatari/build/src/gui-sdl/libGuiSdl.a \
	hatari/build/src/libFloppy.a \
	hatari/build/src/debug/libDebug.a \
	hatari/build/src/libcore.a \
	-lSDL2 \
	-lz
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
	(cd hatari/build && cmake --build . -j)

clean:
	rm -f -r $(BD)
	rm -f -r hatari/build
