CC=gcc
CFLAGS=-O2
LDFLAGS=-shared

BD=build/
CORE=$(BD)hatarib.dll
SOURCES = \
	core/core.c
OBJECTS = $(SOURCES:%.c=$(BD)%.o)
HATARILIBS=hatari/build/src/libcore.a

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
	(cd hatari/build && cmake ..)
	(cd hatari/build && cmake --build . -j)

clean:
	rm -f -r $(BD)
	rm -f -r hatari/build
