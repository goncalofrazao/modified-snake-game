#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ncurses.h>
#include <assert.h>
#include <pthread.h>

#include "../snd_rcv_proto.h"

#define WINDOW_SIZE 30

static void *context;

/**
 * @brief Handles lizard client
 *
 * @param arg server address
 * @return void*
 */
void *handle_lizard(void *arg)
{
    char *server_req = (char *)arg;

    // create requester socket
    void *requester = zmq_socket(context, ZMQ_REQ);
    assert(zmq_connect(requester, server_req) == 0);

    Type msgtype = TYPE__LIZARD_CONNECT;
    void *buffer;
    size_t packed_size;

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

    // check if server is full
    if (reply->success == 0)
    {
        printf("Server is full\n");
        zmq_close(requester);
        free(server_req);
        exit(0);
    }

    // set lizard letter and password
    lizard.id = strdup(reply->id);
    lizard.password = reply->password;
    lizard.has_password = 1;

    msgtype = TYPE__LIZARD_MOVE;
    int key;

    // MAIN LOOP to get key pressed and send move/disconnect message
    while (1)
    {
        // get key pressed
        key = getch();

        // check if key is an arrow key or q or Q
        switch (key)
        {
        case KEY_LEFT:
            lizard.direction = DIRECTION__LEFT;
            break;
        case KEY_RIGHT:
            lizard.direction = DIRECTION__RIGHT;
            break;
        case KEY_DOWN:
            lizard.direction = DIRECTION__DOWN;
            break;
        case KEY_UP:
            lizard.direction = DIRECTION__UP;
            break;
        case 'Q':
        case 'q':
            msgtype = TYPE__LIZARD_DISCONNECT;
            break;
        default:
            continue;
        }

        // send move/disconnect message
        zmq_send(requester, &msgtype, sizeof(Type), ZMQ_SNDMORE);

        lizard.has_direction = 1;
        PACK__REQUEST_MESSAGE(lizard, buffer, packed_size);
        SEND__MESSAGE(requester, buffer, packed_size);

        // receive reply message
        RECV_UNPACK__REPLY_MESSAGE(requester, reply);

        // if q or Q was pressed, exit
        if (key == 'Q' || key == 'q')
        {
            break;
        }
    }

    // cleanup and exit
    zmq_close(requester);
    free(server_req);

    exit(0);
}

/**
 * @brief Handles board and scoreboard display
 *
 * @param arg server address
 * @return void*
 */
void *handle_display(void *arg)
{
    char *server_sub = (char *)arg;

    // create subscriber socket
    void *subscriber = zmq_socket(context, ZMQ_SUB);
    assert(zmq_connect(subscriber, server_sub) == 0);
    zmq_setsockopt(subscriber, ZMQ_SUBSCRIBE, "", 0);

    // initialize ncurses
    initscr();
    clear();
    curs_set(0);
    cbreak();
    keypad(stdscr, TRUE);
    noecho();

    // board window
    WINDOW *board = newwin(WINDOW_SIZE + 2, WINDOW_SIZE + 2, 0, 0);
    box(board, 0, 0);
    wrefresh(board);

    // score board window
    WINDOW *score_board = newwin(WINDOW_SIZE + 2, 20, 0, WINDOW_SIZE + 3);
    box(score_board, 0, 0);
    mvwprintw(score_board, 0, 5, "  Scores  ");
    wrefresh(score_board);

    DisplayUpdateMessage *update;
    char ch;
    // MAIN LOOP to receive update messages and display them
    while (1)
    {
        // receive display update message
        RECV_UNPACK__DISPLAY_UPDATE_MESSAGE(subscriber, update);
        ch = strdup(update->ch)[0];

        // draw lizard
        wmove(board, update->pos_x, update->pos_y);
        waddch(board, ch);
        wrefresh(board);

        // check if is a lizard
        if (ch < 'a' || ch > 'z')
        {
            continue;
        }

        // draw box around board and score board
        box(board, 0, 0);
        box(score_board, 0, 0);
        mvwprintw(score_board, 0, 5, "  Scores  ");
        wrefresh(board);
        wrefresh(score_board);

        // update score
        mvwprintw(score_board, ch - 'a' + 2, 1, "Lizard %c:         ", ch);
        wrefresh(score_board);
        mvwprintw(score_board, ch - 'a' + 2, 11, "%d", update->score);
        wrefresh(score_board);
    }

    // cleanup and exit
    zmq_close(subscriber);
    free(server_sub);
    endwin();

    return 0;
}

/**
 * @brief creates a lizard client and connects to server to play the game
 * displays the board and score board
 *
 * @param argc
 * @param argv server address, request port, subscriber port
 * @return int error code (0 = success)
 */
int main(int argc, char *argv[])
{
    // initialize zmq context
    context = zmq_ctx_new();

    // arguments check
    if (argc != 4)
    {
        printf("Usage: %s <server_address> <req/rep_port> <sub/pub_port>\n", argv[0]);
        return 1;
    }

    // server request address
    char *server_req = (char *)malloc((strlen(argv[1]) + strlen(argv[2]) + 8) * sizeof(char));
    sprintf(server_req, "tcp://%s:%s", argv[1], argv[2]);

    // server subscriber address
    char *server_sub = (char *)malloc((strlen(argv[1]) + strlen(argv[3]) + 8) * sizeof(char));
    sprintf(server_sub, "tcp://%s:%s", argv[1], argv[3]);

    // create threads to handle lizard and display
    pthread_t lizard_thread, display_thread;
    pthread_create(&lizard_thread, NULL, handle_lizard, (void *)server_req);
    pthread_create(&display_thread, NULL, handle_display, (void *)server_sub);

    // wait for threads to finish
    pthread_join(lizard_thread, NULL);
    pthread_join(display_thread, NULL);

    // cleanup and exit
    zmq_ctx_destroy(context);

    return 0;
}
