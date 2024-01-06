all: proto lizard-client roaches-client lizardsnroaches-server

LDFLAGS = -L /opt/homebrew/Cellar/zeromq/4.3.5_1/lib -I /opt/homebrew/Cellar/zeromq/4.3.5_1/include

PROTOFLAGS = -L /opt/homebrew/Cellar/protobuf-c/1.5.0/lib -I /opt/homebrew/Cellar/protobuf-c/1.5.0/include

proto: lar-defs.proto
	protoc-c --c_out=. lar-defs.proto

lizard-client: lizard/lizard-client.c lar-defs.h
	gcc lizard/lizard-client.c lar-defs.pb-c.c -o lizard/lizard-client -lncurses -lzmq -lprotobuf-c $(LDFLAGS) $(PROTOFLAGS)
	
roaches-client: roaches/roaches-client.c lar-defs.h
	gcc roaches/roaches-client.c lar-defs.pb-c.c -o roaches/roaches-client -lncurses -lzmq -lprotobuf-c $(LDFLAGS) $(PROTOFLAGS)

lizardsnroaches-server: server/lizardsnroaches-server.c lar-defs.h
	gcc server/lizardsnroaches-server.c lar-defs.pb-c.c -o server/lizardsnroaches-server -lncurses -lzmq -lprotobuf-c $(LDFLAGS) $(PROTOFLAGS)

clean:
	rm -rf lizard/lizard-client roaches/roaches-client display/display-app server/lizardsnroaches-server *.dSYM
