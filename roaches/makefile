LDFLAGS = -L /opt/homebrew/Cellar/zeromq/4.3.5_1/lib
CFLAGS = -I /opt/homebrew/Cellar/zeromq/4.3.5_1/include

roaches-client: roaches-client.c ../lar-defs.h
	gcc roaches-client.c -o roaches-client -lncurses -lzmq $(LDFLAGS) $(CFLAGS)

clean:
	rm -rf roaches-client *.dSYM