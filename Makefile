ALL=gw rtape shutdown mlftp

MLDEV=mldev/mldev.o mldev/protoc.o mldev/io-chaos.o
LIBWORD=dasm/libword/libword

all: $(ALL)

CFLAGS=-Wall -W -g -Idasm/libword

gw: gw.o chaos.o
	$(CC) $(LDFLAGS) -o $@ $^ $(LDLIBS)

mlftp: mlftp.o chaos.o $(MLDEV) $(LIBWORD).a
	$(CC) $(LDFLAGS) -o $@ $^ $(LDLIBS) $(LIBWORD).a

rtape: rtape.o chaos.o tape-image.o
	$(CC) $(LDFLAGS) -o $@ $^ $(LDLIBS)

shutdown: shutdown.o chaos.o
	$(CC) $(LDFLAGS) -o $@ $^ $(LDLIBS)

dasm/libword:
	git submodule sync
	git submodule update --init

$(LIBWORD).a $(LIBWORD).h: dasm/libword
	cd dasm/libword && $(MAKE)

install: $(ALL)
	./install.sh $^

clean:
	rm -f *.o

gw.o:: chaos.h
chaos.o:: chaos.h
mlftp.o:: chaos.h mldev/mldev.h mldev/protoc.h mldev/io.h $(LIBWORD).h
rtape.o:: chaos.h tape-image.h
shutdown.o:: chaos.h
tape-image.o:: tape-image.h
