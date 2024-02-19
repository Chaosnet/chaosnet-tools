ALL=gw rtape recread shutdown

all: $(ALL)

CFLAGS=-Wall -W -g

gw: gw.o chaos.o
	$(CC) $(LDFLAGS) -o $@ $^ $(LDLIBS)

rtape: rtape.o chaos.o tape-image.o
	$(CC) $(LDFLAGS) -o $@ $^ $(LDLIBS)

recread: recread.o tape-image.o
	$(CC) $(LDFLAGS) -o $@ $^ $(LDLIBS)

shutdown: shutdown.o chaos.o
	$(CC) $(LDFLAGS) -o $@ $^ $(LDLIBS)

install: $(ALL)
	./install.sh $^

clean:
	rm -f *.o

gw.o:: chaos.h
chaos.o:: chaos.h
rtape.o:: chaos.h tape-image.h
shutdown.o:: chaos.h
tape-image.o:: tape-image.h
