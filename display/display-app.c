#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ncurses.h>
#include <assert.h>
#include <zmq.h>

#include "../lar-defs.h"

int main(int argc, char *argv[]) {
    display_t display;
    
    // arguments check
    if (argc != 3) {
        printf("Usage: %s <server_address> <pub/sub_port>\n", argv[0]);
        return 1;
    }

    // server connection
    char *server_sub = (char*) malloc((strlen(argv[1]) + strlen(argv[2]) + 8) * sizeof(char));
    sprintf(server_sub, "tcp://%s:%s", argv[1], argv[2]);
    void* context = zmq_ctx_new();
    void* subscriber = zmq_socket(context, ZMQ_SUB);
    assert(zmq_connect(subscriber, server_sub) == 0);
    zmq_setsockopt(subscriber, ZMQ_SUBSCRIBE, "", 0);

    // initialize ncurses
    initscr();
    curs_set(0);
	cbreak();
    keypad(stdscr, TRUE);
	noecho();
    
    // board window
    WINDOW *board = newwin(WINDOW_SIZE + 2, WINDOW_SIZE + 2, 0, 0);
    box(board, 0 , 0);	
	wrefresh(board);

    // score board window
    WINDOW * score_board = newwin(WINDOW_SIZE + 2, 20, 0, WINDOW_SIZE + 3);
    box(score_board, 0 , 0);
    mvwprintw(score_board, 0, 5, "  Scores  ");
    wrefresh(score_board);

    // main loop to receive update messages and display changes
    while (1) {
        // receive update message
        assert(zmq_recv(subscriber, &display, sizeof(display_t), 0) == sizeof(display_t));
        wmove(board, display.pos_x, display.pos_y);
        waddch(board, display.ch | A_BOLD);
        wrefresh(board);

        // update score
        mvwprintw(score_board, display.ch - 'a' + 2, 1, "Lizard %c:     ", display.ch);
        wrefresh(score_board);
        mvwprintw(score_board, display.ch - 'a' + 2, 11, "%d", display.score);
        wrefresh(score_board);
    }

    // cleanup and exit
    zmq_close(subscriber);
    zmq_ctx_destroy(context);
    free(server_sub);
    endwin();

    return 0;
}