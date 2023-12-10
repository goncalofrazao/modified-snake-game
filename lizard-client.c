#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ncurses.h>

#include "lar-defs.h"
#include "zhelpers.h"

int main(int argc, char *argv[]) {
    msg_t lizard;
    reply_t reply;
    int key;
    int n = 0;

    if (argc != 3) {
        printf("Usage: %s <server_address> <req/rep_port>\n", argv[0]);
        return 1;
    }

    char *server_endpoint = (char*) malloc((strlen(argv[1]) + strlen(argv[2]) + 8) * sizeof(char));
    sprintf(server_endpoint, "tcp://%s:%s", argv[1], argv[2]);

    // Connect to reply socket
    void *context = zmq_ctx_new();
    void *requester = zmq_socket(context, ZMQ_REQ);
    assert(zmq_connect(requester, server_endpoint) == 0);

    //Send connection message
    lizard.type = LIZARD_CONNECT;
    assert(zmq_send(requester, &lizard, sizeof(msg_t), 0) == sizeof(msg_t));

    //Receive reply with assigned letter and password
    assert(zmq_recv(requester, &reply, sizeof(reply_t), 0) == sizeof(reply_t));
    lizard.id = reply.id;
    lizard.password = reply.password;
    lizard.type = LIZARD_MOVE;

    //Check if server is full
    if (lizard.password == -1){
        printf("Server is full\n");
        zmq_close(requester);
        zmq_ctx_destroy(context);
        free(server_endpoint);
        return 0;
    }

    //Initialize ncurses
    initscr();
    cbreak();
    keypad(stdscr, TRUE);
    noecho();

    while (1) {
        key = getch();	
        n++;
        
        switch (key){
        case KEY_LEFT:
            mvprintw(0,0,"                         ");
            mvprintw(0,0,"Left arrow is pressed");
            refresh();
            lizard.direction = LEFT;
            break;
        case KEY_RIGHT:
            mvprintw(0,0,"                         ");
            mvprintw(0,0,"Right arrow is pressed");
            refresh();
            lizard.direction = RIGHT;
            break;
        case KEY_DOWN:
            mvprintw(0,0,"                         ");
            mvprintw(0,0,"Down arrow is pressed");
            refresh();
            lizard.direction = DOWN;
            break;
        case KEY_UP:
            mvprintw(0,0,"                         ");
            mvprintw(0,0,"Up arrow is pressed");
            refresh();
            lizard.direction = UP;
            break;
        case 'Q':
        case 'q':
            mvprintw(0,0,"                         ");
            mvprintw(0,0,"Disconnecting from server");
            refresh();
            lizard.type = LIZARD_DISCONNECT;
            break;
        }
        assert(zmq_send(requester, &lizard, sizeof(msg_t), 0) == sizeof(msg_t));
        assert(zmq_recv(requester, &reply, sizeof(reply_t), 0) == sizeof(reply_t));
        if (key == 'Q' || key == 'q'){
            break;
        }
        mvprintw(1,0, "          ");
        mvprintw(1,0, "Score: %d", reply.score);
        refresh();
    }

    zmq_close(requester);
    zmq_ctx_destroy(context);
    free(server_endpoint);

    return 0;
}
