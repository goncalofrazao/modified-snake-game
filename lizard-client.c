#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ncurses.h>

#include "lar-defs.h"
#include "zhelpers.h"

int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("Usage: %s <server_address> <req/rep_port>\n", argv[0]);
        return 1;
    }

    // Get Server Address and Port
    char *server_address = argv[1];
    char *server_port = argv[2];

    char *server_endpoint = (char*) malloc((strlen(server_address) + strlen(server_port) + 8) * sizeof(char));
    sprintf(server_endpoint, "tcp://%s:%s", server_address, server_port);

    // Connect to reply socket
    void *context = zmq_ctx_new();
    void *requester = zmq_socket(context, ZMQ_REQ);
    int rc = zmq_connect(requester, server_endpoint);
    assert(rc == 0);

    //Send connection message
    msg_t lizard;
    lizard.type = LIZARD_CONNECT;
    zmq_send(requester, &lizard, sizeof(msg_t), 0);

    //Receive reply with assigned letter and password
    reply_t reply;
    zmq_recv(requester, &reply, sizeof(reply_t), 0);
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

    int key;
    int n = 0;
    while (1) {
        key = getch();	

        n++;
        switch (key){
        case KEY_LEFT:
            mvprintw(0,0,"                         ");
            refresh();
            mvprintw(0,0,"Left arrow is pressed");
            refresh();
            lizard.direction = LEFT;
            break;
        case KEY_RIGHT:
            mvprintw(0,0,"                         ");
            refresh();
            mvprintw(0,0,"Right arrow is pressed");
            refresh();
            lizard.direction = RIGHT;
            break;
        case KEY_DOWN:
            mvprintw(0,0,"                         ");
            refresh();
            mvprintw(0,0,"Down arrow is pressed");
            refresh();
            lizard.direction = DOWN;
            break;
        case KEY_UP:
            mvprintw(0,0,"                         ");
            refresh();
            mvprintw(0,0,"Up arrow is pressed");
            refresh();
            lizard.direction = UP;
            break;
        case 'Q':
        case 'q':
            mvprintw(0,0,"                         ");
            refresh();
            mvprintw(0,0,"Disconnecting from server");
            refresh();
            lizard.type = LIZARD_DISCONNECT; 
            break;
        }
            zmq_send(requester, &lizard, sizeof(msg_t), 0);
            zmq_recv(requester, &reply, sizeof(reply_t), 0);
            if (key == 'Q' || key == 'q'){
                break;
            }
            mvprintw(1,0, "          ");
            refresh();
            mvprintw(1,0, "Score: %d", reply.score);
            refresh();
    }

    zmq_close(requester);
    zmq_ctx_destroy(context);
    free(server_endpoint);

    return 0;
}
