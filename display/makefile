LDFLAGS = -L /opt/homebrew/Cellar/zeromq/4.3.5_1/lib
CFLAGS = -I /opt/homebrew/Cellar/zeromq/4.3.5_1/include

display-app: display-app.c ../lar-defs.h
	gcc display-app.c -o display-app -lncurses -lzmq $(LDFLAGS) $(CFLAGS)

clean:
	rm -rf display-app *.dSYM