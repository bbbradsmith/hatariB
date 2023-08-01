CC=gcc
CFLAGS=
LDFLAGS=-shared

BD=build/
CORE=$(BD)hatarib.dll
SOURCES = \
	core/core.c
OBJECTS = $(SOURCES:%.c=$(BD)%.o)

default: directories $(CORE)

directories:
	mkdir -p $(BD)
	mkdir -p $(BD)core/

$(CORE): $(OBJECTS)
	$(CC) -o $(CORE) $(OBJECTS) $(LDFLAGS)

$(BD)core/%.o: core/%.c
	$(CC) $(CFLAGS) -o $@ -c $< 

clean:
	rm $(OBJECTS)
	rm $(CORE)
