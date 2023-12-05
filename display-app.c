#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ncurses.h>

#include "zhelpers.h"
#include "lar-defs.h"

int main(int argc, char *argv[]) {
    if (argc != 4) {
        printf("Usage: %s <server_address> <req/rep_port> <pub/sub_port>\n", argv[0]);
        return 1;
    }

    // Get Server Address and Port
    char *server_address = argv[1];
    char *server_req_port = argv[2];
    char *server_pub_port = argv[3];

    char *server_req = (char*) malloc((strlen(server_address) + strlen(server_req_port) + 8) * sizeof(char));
    sprintf(server_req, "tcp://%s:%s", server_address, server_req_port);

    char *server_sub = (char*) malloc((strlen(server_address) + strlen(server_pub_port) + 8) * sizeof(char));
    sprintf(server_sub, "tcp://%s:%s", server_address, server_pub_port);

    // Connect the subscriber socket to the publisher
    void* context = zmq_ctx_new();
    void* subscriber = zmq_socket(context, ZMQ_SUB);
    int rc_s = zmq_connect(subscriber, server_pub_port);
    assert(rc_s == 0);
    zmq_setsockopt(subscriber, ZMQ_SUBSCRIBE, "", 0);

    // Connect the requester socket to to send connect message
    void* requester = zmq_socket(context, ZMQ_REQ);
    int rc_r = zmq_connect(requester, server_req_port);
    assert(rc_r == 0);

    // Send connect message and receive window size
    msg_t msg;
    int window_size;
    msg.type = DISPLAY_CONNECT;
    zmq_send(requester, &msg, sizeof(msg_t), 0);
    zmq_recv(requester, &window_size, sizeof(int), 0);

    // Initialize ncurses
    initscr();
    curs_set(0);
	cbreak();
    keypad(stdscr, TRUE);
	noecho();

    /* creates a window and draws a border */
    WINDOW * my_win = newwin(window_size, window_size, 0, 0);
    box(my_win, 0 , 0);	
	wrefresh(my_win);

    // Receive amount of characters to draw
    int amount_of_recv;
    display_t display;
    zmq_recv(requester, &amount_of_recv, sizeof(int), 0);
    for (int i = 0; i < amount_of_recv; i++) {
        zmq_recv(requester, &display, sizeof(display), 0);
        wmove(my_win, display.pos_x, display.pos_y);
        waddch(my_win, display.ch | A_BOLD);
        wrefresh(my_win);
    }

    // Main loop to receive and display messages
    while (1) {
        //Receive update message
        zmq_recv(subscriber, &display, sizeof(display_t), 0);
        wmove(my_win, display.pos_x, display.pos_y);
        waddch(my_win, display.ch | A_BOLD);
        wrefresh(my_win);
    }

    // Cleanup and exit
    zmq_close(requester);
    zmq_close(subscriber);
    zmq_ctx_destroy(context);
    free(server_req_port);
    free(server_pub_port);
    endwin();

    return 0;
}