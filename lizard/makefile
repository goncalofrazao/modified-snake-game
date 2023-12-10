LDFLAGS = -L /opt/homebrew/Cellar/zeromq/4.3.5_1/lib
CFLAGS = -I /opt/homebrew/Cellar/zeromq/4.3.5_1/include

lizard-client: lizard-client.c ../lar-defs.h
	gcc lizard-client.c -o lizard-client -lncurses -lzmq $(LDFLAGS) $(CFLAGS)

clean:
	rm -rf lizard-client *.dSYM