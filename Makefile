all: lizard-client roaches-client wasps-client lizardsnroaches-server

LDFLAGS = -L /opt/homebrew/Cellar/zeromq/4.3.5_1/lib -I /opt/homebrew/Cellar/zeromq/4.3.5_1/include

PROTOFLAGS = -L /opt/homebrew/Cellar/protobuf-c/1.5.0/lib -I /opt/homebrew/Cellar/protobuf-c/1.5.0/include

proto: lar-defs.proto
	protoc-c --c_out=. lar-defs.proto
	protoc --python_out=./roaches lar-defs.proto

lizard-client: lizard/lizard-client.c
	gcc lizard/*.c lar-defs.pb-c.c -o lizard-client -lncurses -lzmq -lprotobuf-c -lpthread $(LDFLAGS) $(PROTOFLAGS)
	
roaches-client: roaches/roaches-client.c
	gcc roaches/*.c lar-defs.pb-c.c -o roaches-client -lncurses -lzmq -lprotobuf-c $(LDFLAGS) $(PROTOFLAGS)

lizardsnroaches-server: server/lizardsnroaches-server.c
	gcc server/*.c lar-defs.pb-c.c -g -o lizardsnroaches-server -lncurses -lzmq -lprotobuf-c -lpthread $(LDFLAGS) $(PROTOFLAGS)

wasps-client: wasps/wasps-client.c
	gcc wasps/*.c lar-defs.pb-c.c -o wasps-client -lncurses -lzmq -lprotobuf-c $(LDFLAGS) $(PROTOFLAGS)

clean:
	rm -rf lizard-client roaches-client wasps-client lizardsnroaches-server *.dSYM

