ALL=gw shutdown

all: $(ALL)

CFLAGS=-Wall -W -g

gw: gw.o chaos.o
	$(CC) $(LDFLAGS) -o $@ $^ $(LDLIBS)

shutdown: shutdown.o chaos.o
	$(CC) $(LDFLAGS) -o $@ $^ $(LDLIBS)

install: $(ALL)
	./install.sh $^

clean:
	rm -f *.o

gw.o:: chaos.h
chaos.o:: chaos.h
shutdown.o:: chaos.h
