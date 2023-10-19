all: gw

CFLAGS=-Wall -W -g

gw: gw.o chaos.o chaos.h
	$(CC) $(LDFLAGS) -o $@ $^ $(LDLIBS)

clean:
	rm -f *.o

gw.o:: chaos.h
chaos.o:: chaos.h
