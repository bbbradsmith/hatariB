CC=gcc
CFLAGS=-O2
LDFLAGS=-shared

BD=build/
CORE=$(BD)hatarib.dll
SOURCES = \
	core/core.c
OBJECTS = $(SOURCES:%.c=$(BD)%.o)

default: directories $(CORE) hatarilib

directories:
	mkdir -p $(BD)
	mkdir -p $(BD)core
	mkdir -p hatari/build

$(CORE): $(OBJECTS)
	$(CC) -o $(CORE) $(OBJECTS) $(LDFLAGS)

$(BD)core/%.o: core/%.c
	$(CC) $(CFLAGS) -o $@ -c $< 

hatarilib: directories
	(cd hatari/build && cmake ..)
	(cd hatari/build && cmake --build . -j 4)

clean:
	rm -f -r $(BD)
	rm -f -r hatari/build
