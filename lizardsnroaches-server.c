#include <ncurses.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>  
#include <stdlib.h>

#include "lar-defs.h"
#include "zhelpers.h"

#define WINDOW_SIZE 30
#define LIZARDS_NUMBER 26
#define ROACHES_NUMBER WINDOW_SIZE*WINDOW_SIZE/3

typedef struct info_t
{
    char id;
    int password;
    int points;
    int pos_x, pos_y;
    direction_t direction;
} info_t;

void new_position(info_t *move, direction_t direction){
    move->direction = direction;
    switch (move->direction) {
    case UP:
        (move->pos_x)--;
        if(move->pos_x == 0)
            move->pos_x = 1;
        break;
    case DOWN:
        (move->pos_x)++;
        if(move->pos_x == WINDOW_SIZE + 1)
            move->pos_x = WINDOW_SIZE;
        break;
    case LEFT:
        (move->pos_y)--;
        if(move->pos_y == 0)
            move->pos_y = 1;
        break;
    case RIGHT:
        (move->pos_y)++;
        if(move->pos_y == WINDOW_SIZE + 1)
            move->pos_y = WINDOW_SIZE;
    }
}

void init_lizard(info_t *lizard_data, int lizard, msg_t *msg) {
    lizard_data->id = lizard + 'a';
    lizard_data->password = rand();
    lizard_data->points = 0;
    lizard_data->pos_x = rand() % WINDOW_SIZE + 1;
    lizard_data->pos_y = rand() % WINDOW_SIZE + 1;
    lizard_data->direction = rand() % 4;
}

void init_roach(info_t *roach_data, int *roach, msg_t *msg) {
    roach_data->id = msg->id;
    roach_data->password = rand();
    roach_data->points = atoi(&msg->id);
    roach_data->pos_x = rand() % WINDOW_SIZE + 1;
    roach_data->pos_y = rand() % WINDOW_SIZE + 1;
    (*roach)++;
}

void publisher_update(info_t *move, char ch, void *publisher) {
    display_t display_info;
    display_info.pos_x = move->pos_x;
    display_info.pos_y = move->pos_y;
    display_info.ch = ch;
    zmq_send(publisher, &display_info, sizeof(display_t), 0);
}

int find_roach(info_t roach_data[], int roaches, msg_t msg) {
    for(int i = 0; i < roaches; i++) {
        if(roach_data[i].password == msg.password) {
            return i;
        }
    }
    return -1;
}

int find_lizard(info_t lizard_data[]) {
    for(int i = 0; i < LIZARDS_NUMBER; i++) {
        if(lizard_data[i].id == '0') {
            return i;
        }
    }
    return -1;
}

void invalid_msg(void *responder) {
    reply_t reply;
    reply.id = rand() % 256;
    reply.password = -1;
    zmq_send(responder, &reply, sizeof(reply_t), 0);
}

// int points(info_t *lizard, WINDOW *my_win, info_t roach_data[], int roaches) {
//     info_t aux;
//     memcpy(&aux, lizard, sizeof(info_t));
//     new_position(&aux, aux.direction);
//     return 0;
// }

void draw_lizard(void *publisher, info_t *lizard, WINDOW *my_win, int delete) {
    info_t aux;
    memcpy(&aux, lizard, sizeof(info_t));
    char head = delete ? ' ' : lizard->id;

    switch (lizard->direction)
    {
    case LEFT:
        aux.direction = RIGHT;
        break;
    case RIGHT:
        aux.direction = LEFT;
        break;
    case UP:
        aux.direction = DOWN;
        break;
    default:
        aux.direction = UP;
    }
    
    if (delete) {
        aux.id = ' ';
    }
    else if (aux.points < 50) {
        aux.id = '.';
    }
    else {
        aux.id = '*';
    }

    wmove(my_win, aux.pos_x, aux.pos_y);
    waddch(my_win, head | A_BOLD);
    publisher_update(&aux, head, publisher);

    for (int i = 0; i < 5; i++) {
        new_position(&aux, aux.direction);
        wmove(my_win, aux.pos_x, aux.pos_y);
        waddch(my_win, aux.id | A_BOLD);
        publisher_update(&aux, aux.id, publisher);
    }
}

void draw_roach(void *publisher, info_t *roach, WINDOW *my_win, int delete) {
    char head = delete ? ' ' : roach->id;
    wmove(my_win, roach->pos_x, roach->pos_y);
    waddch(my_win, head | A_BOLD);
    publisher_update(roach, head, publisher);
}

int main() {
    display_t display_info;
    msg_t msg;
    reply_t reply;
    info_t lizard_data[LIZARDS_NUMBER];
    info_t roach_data[ROACHES_NUMBER];
    int roaches = 0;
    int roach;
    int lizard;
    info_t *move;

    srand(time(NULL));

    for(int i = 0; i < LIZARDS_NUMBER; i++) {
        lizard_data[i].id = '0';
    }
    
    // open server sockets
	void *context = zmq_ctx_new ();
    void *responder = zmq_socket (context, ZMQ_REP);
    zmq_bind (responder, "tcp://*:5555");

    void *publisher = zmq_socket (context, ZMQ_PUB);
    zmq_bind (publisher, "tcp://*:5556");

	initscr();
	cbreak();
    curs_set(0);
    keypad(stdscr, TRUE);
	noecho();

    // creates a window and draws a border
    WINDOW * my_win = newwin(WINDOW_SIZE + 2, WINDOW_SIZE + 2, 0, 0);
    box(my_win, 0 , 0);
	wrefresh(my_win);
    
    while (1)
    {
        zmq_recv(responder, &msg, sizeof(msg_t), 0);
        switch (msg.type) {
        case LIZARD_CONNECT:
            if ((lizard = find_lizard(lizard_data)) == -1) {
                invalid_msg(responder);
                continue;
            }
            move = &lizard_data[lizard];
            init_lizard(move, lizard, &msg);
            draw_lizard(publisher, move, my_win, 0);
            reply.id = move->id; reply.password = move->password;
            break;
        case ROACH_CONNECT:
            if (roaches == ROACHES_NUMBER) {
                invalid_msg(responder);
                continue;
            }
            move = &roach_data[roaches];
            init_roach(move, &roaches, &msg);
            draw_roach(publisher, move, my_win, 0);
            reply.id = move->id; reply.password = move->password;
            break;
        case LIZARD_MOVE:
            if (lizard_data[msg.id - 'a'].password != msg.password) {
                invalid_msg(responder);
                continue;
            }
            move = &lizard_data[msg.id - 'a'];
            draw_lizard(publisher, move, my_win, 1);
            new_position(move, msg.direction);
            draw_lizard(publisher, move, my_win, 0);
            break;
        case ROACH_MOVE:
            roach = find_roach(roach_data, roaches, msg);
            if (roach == -1) {
                invalid_msg(responder);
                continue;
            }
            move = &roach_data[roach];
            draw_roach(publisher, move, my_win, 1);
            new_position(move, msg.direction);
            draw_roach(publisher, move, my_win, 0);
            break;
        case LIZARD_DISCONNECT:
            if (lizard_data[msg.id - 'a'].password == msg.password) {
                move = &lizard_data[msg.id - 'a'];
                draw_lizard(publisher, move, my_win, 1);
                lizard_data[msg.id - 'a'].id = '0';
            }
            break;
        case DISPLAY_CONNECT:
            // TODO
            continue;
        }
        wrefresh(my_win);
        zmq_send(responder, &reply, sizeof(reply_t), 0);
    }
  	endwin();

    zmq_close (responder);
    zmq_close (publisher);
    zmq_ctx_destroy (context);

	return 0;
}