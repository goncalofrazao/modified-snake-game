all: lizard-client roaches-client display-app lizardsnroaches-server

lizard-client: lizard-client.c zhelpers.h lar-defs.h
	gcc lizard-client.c -o lizard-client -lncurses -lzmq -L /opt/homebrew/Cellar/zeromq/4.3.5_1/lib -I /opt/homebrew/Cellar/zeromq/4.3.5_1/include
	
roaches-client: roaches-client.c zhelpers.h lar-defs.h
	gcc roaches-client.c -o roaches-client -lncurses -lzmq -L /opt/homebrew/Cellar/zeromq/4.3.5_1/lib -I /opt/homebrew/Cellar/zeromq/4.3.5_1/include

display-app: display-app.c zhelpers.h lar-defs.h
	gcc display-app.c -o display-app -lncurses -lzmq -L /opt/homebrew/Cellar/zeromq/4.3.5_1/lib -I /opt/homebrew/Cellar/zeromq/4.3.5_1/include

lizardsnroaches-server: lizardsnroaches-server.c zhelpers.h lar-defs.h
	gcc lizardsnroaches-server.c -o lizardsnroaches-server -lncurses -lzmq -L /opt/homebrew/Cellar/zeromq/4.3.5_1/lib -I /opt/homebrew/Cellar/zeromq/4.3.5_1/include

clean:
	rm -rf lizard-client roaches-client display-app lizardsnroaches-server *.dSYM