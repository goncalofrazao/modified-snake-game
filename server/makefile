LDFLAGS = -L /opt/homebrew/Cellar/zeromq/4.3.5_1/lib
CFLAGS = -I /opt/homebrew/Cellar/zeromq/4.3.5_1/include

lizardsnroaches-server: lizardsnroaches-server.c ../lar-defs.h
	gcc lizardsnroaches-server.c -o lizardsnroaches-server -lncurses -lzmq $(LDFLAGS) $(CFLAGS)

clean:
	rm -rf lizardsnroaches-server *.dSYM