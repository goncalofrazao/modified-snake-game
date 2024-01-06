#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ncurses.h>
#include <assert.h>

#include "../snd_rcv_proto.h"

int main(int argc, char *argv[])
{

    Type msgtype = TYPE__LIZARD_CONNECT;
    void *buffer;
    size_t packed_size;

    int key;
    int n = 0;

    // arguments check
    if (argc != 3)
    {
        printf("Usage: %s <server_address> <req/rep_port>\n", argv[0]);
        return 1;
    }

    // server connection
    char *server_endpoint = (char *)malloc((strlen(argv[1]) + strlen(argv[2]) + 8) * sizeof(char));
    sprintf(server_endpoint, "tcp://%s:%s", argv[1], argv[2]);
    void *context = zmq_ctx_new();
    void *requester = zmq_socket(context, ZMQ_REQ);
    assert(zmq_connect(requester, server_endpoint) == 0);

    // send connection message
    zmq_send(requester, &msgtype, sizeof(Type), ZMQ_SNDMORE);

    RequestMessage lizard = REQUEST_MESSAGE__INIT;
    lizard.has_direction = 0;
    lizard.has_password = 0;
    PACK__REQUEST_MESSAGE(lizard, buffer, packed_size);
    SEND__MESSAGE(requester, buffer, packed_size);

    // receive reply with assigned letter and password
    ReplyMessage *reply;
    RECV_UNPACK__REPLY_MESSAGE(requester, reply);

    lizard.id = strdup(reply->id);
    lizard.password = reply->password;
    lizard.has_password = 1;

    // check if server is full
    if (reply->success == 0)
    {
        printf("Server is full\n");
        zmq_close(requester);
        zmq_ctx_destroy(context);
        free(server_endpoint);
        return 0;
    }

    // initialize ncurses
    initscr();
    cbreak();
    keypad(stdscr, TRUE);
    noecho();

    msgtype = TYPE__LIZARD_MOVE;
    while (1)
    {
        // get key pressed
        key = getch();
        n++;

        switch (key)
        {
        case KEY_LEFT:
            mvprintw(0, 0, "                         ");
            mvprintw(0, 0, "Left arrow is pressed");
            refresh();
            lizard.direction = DIRECTION__LEFT;
            break;
        case KEY_RIGHT:
            mvprintw(0, 0, "                         ");
            mvprintw(0, 0, "Right arrow is pressed");
            refresh();
            lizard.direction = DIRECTION__RIGHT;
            break;
        case KEY_DOWN:
            mvprintw(0, 0, "                         ");
            mvprintw(0, 0, "Down arrow is pressed");
            refresh();
            lizard.direction = DIRECTION__DOWN;
            break;
        case KEY_UP:
            mvprintw(0, 0, "                         ");
            mvprintw(0, 0, "Up arrow is pressed");
            refresh();
            lizard.direction = DIRECTION__UP;
            break;
        case 'Q':
        case 'q':
            mvprintw(0, 0, "                         ");
            mvprintw(0, 0, "Disconnecting from server");
            refresh();
            msgtype = TYPE__LIZARD_DISCONNECT;
            break;
        }

        // send move/disconnect message
        zmq_send(requester, &msgtype, sizeof(Type), ZMQ_SNDMORE);

        lizard.has_direction = 1;
        PACK__REQUEST_MESSAGE(lizard, buffer, packed_size);
        SEND__MESSAGE(requester, buffer, packed_size);

        // receive reply with score
        RECV_UNPACK__REPLY_MESSAGE(requester, reply);

        if (key == 'Q' || key == 'q')
        {
            break;
        }
        mvprintw(1, 0, "          ");
        mvprintw(1, 0, "Score: %d", reply->score);
        refresh();
    }

    // cleanup and exit
    zmq_close(requester);
    zmq_ctx_destroy(context);
    free(server_endpoint);

    return 0;
}
