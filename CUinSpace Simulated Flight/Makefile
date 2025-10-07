TARGET = p2
CC = gcc
CFLAGS = -g -Wall -Wextra -Werror -fsanitize=address
LFLAGS = -pthread -fsanitize=address
SOURCES = src/main.c src/display.c src/manager.c src/resource.c src/system.c src/event.c
OBJECTS = main.o display.o manager.o resource.o system.o event.o

all: $(TARGET)
$(TARGET): $(OBJECTS)
	$(CC) -o $(TARGET) $(OBJECTS) $(LFLAGS)

main.o: src/main.c src/defs.h
	$(CC) -c src/main.c $(CFLAGS)

display.o: src/display.c src/defs.h
	$(CC) -c src/display.c $(CFLAGS)

manager.o: src/manager.c src/defs.h
	$(CC) -c src/manager.c $(CFLAGS)

resource.o: src/resource.c src/defs.h
	$(CC) -c src/resource.c $(CFLAGS)

system.o: src/system.c src/defs.h
	$(CC) -c src/system.c $(CFLAGS)

event.o: src/event.c src/defs.h
	$(CC) -c src/event.c $(CFLAGS)

.PHONY: all clean

clean:
	rm -f $(TARGET) $(OBJECTS)