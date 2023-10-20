all: gw rtape

CFLAGS=-Wall -W -g

gw: gw.o chaos.o chaos.h
	$(CC) $(LDFLAGS) -o $@ $^ $(LDLIBS)

rtape: rtape.o chaos.o tape-image.o
	$(CC) $(LDFLAGS) -o $@ $^ $(LDLIBS)

clean:
	rm -f *.o

gw.o:: chaos.h
chaos.o:: chaos.h
rtape.o:: chaos.h tape-image.h
tape-image.o:: tape-image.h
