TARGET = sender
LIBS = -lrabbitmq
CC = gcc
CFLAGS = -g -Wall

.PHONY: default all clean distclean

default: $(TARGET)
all: default

OBJECTS = $(patsubst %.c, %.o, $(wildcard *.c))
HEADERS = $(wildcard *.h)

%.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@

$(TARGET): $(OBJECTS)
	$(CC) $(OBJECTS) $(CFLAGS) $(LIBS) -o $@

clean:
	-rm -f *.o

distclean: clean
	-rm -f $(TARGET)