TARGETS = sender consumer
LIBS = -lrabbitmq -llz4 $(shell pkg-config --libs libmongo-client)
CC = gcc
CFLAGS = -g -Wall $(shell pkg-config --cflags libmongo-client)

.PHONY: default all clean distclean

default: $(TARGETS)
all: default

OBJECTS = $(patsubst %.c, %.o, $(wildcard *.c))
HEADERS = $(wildcard *.h)

%.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@

consumer: utils.o consumer.o
	$(CC) $^ $(CFLAGS) $(LIBS) -o $@

sender: utils.o sender.o
	$(CC) $^ $(CFLAGS) $(LIBS) -o $@

clean:
	-rm -f *.o

distclean: clean
	-rm -f $(TARGETS)
