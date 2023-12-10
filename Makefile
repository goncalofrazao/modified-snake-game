all: lizard-client roaches-client display-app lizardsnroaches-server

LDFLAGS = -L /opt/homebrew/Cellar/zeromq/4.3.5_1/lib
CFLAGS = -I /opt/homebrew/Cellar/zeromq/4.3.5_1/include

lizard-client: lizard/lizard-client.c lar-defs.h
	gcc lizard/lizard-client.c -o lizard/lizard-client -lncurses -lzmq $(LDFLAGS) $(CFLAGS)
	
roaches-client: roaches/roaches-client.c lar-defs.h
	gcc roaches/roaches-client.c -o roaches/roaches-client -lncurses -lzmq $(LDFLAGS) $(CFLAGS)

display-app: display/display-app.c lar-defs.h
	gcc display/display-app.c -o display/display-app -lncurses -lzmq $(LDFLAGS) $(CFLAGS)

lizardsnroaches-server: server/lizardsnroaches-server.c lar-defs.h
	gcc server/lizardsnroaches-server.c -o server/lizardsnroaches-server -lncurses -lzmq $(LDFLAGS) $(CFLAGS)

clean:
	rm -rf lizard/lizard-client roaches/roaches-client display/display-app server/lizardsnroaches-server *.dSYM