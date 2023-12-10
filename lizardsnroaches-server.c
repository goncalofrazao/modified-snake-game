#include <ncurses.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>  
#include <stdlib.h>
#include <dlfcn.h>

#include "lar-defs.h"
#include "zhelpers.h"

#define LIZARDS_NUMBER 26
#define ROACHES_NUMBER WINDOW_SIZE*WINDOW_SIZE/3

typedef struct info_t
{
    char id;
    int password;
    int points;
    int pos_x, pos_y;
    direction_t direction;
    time_t eaten;
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

int lizard_here(char ch, int pos_x, int pos_y, info_t lizard_data[]) {
    for(int i = 0; i < LIZARDS_NUMBER; i++) {
        if(lizard_data[i].id != '0' && lizard_data[i].id != ch && lizard_data[i].pos_x == pos_x && lizard_data[i].pos_y == pos_y) {
            return 1;
        }
    }
    return 0;
}

void init_lizard(info_t *lizard, info_t lizard_data[], int id, msg_t *msg) {
    lizard->id = id + 'a';
    lizard->password = rand();
    lizard->points = 0;
    do
    {
        lizard->pos_x = rand() % WINDOW_SIZE + 1;
        lizard->pos_y = rand() % WINDOW_SIZE + 1;
    } while (lizard_here(lizard->id, lizard->pos_x, lizard->pos_y, lizard_data));
    
    lizard->direction = rand() % 4;
}

void init_roach(info_t *roach_data, info_t lizard_data[], int *roach, msg_t *msg) {
    roach_data->id = msg->id;
    roach_data->password = rand();
    roach_data->points = atoi(&msg->id);
    do
    {
        roach_data->pos_x = rand() % WINDOW_SIZE + 1;
        roach_data->pos_y = rand() % WINDOW_SIZE + 1;
    } while (lizard_here(roach_data->id, roach_data->pos_x, roach_data->pos_y, lizard_data));
    
    roach_data->eaten = time(NULL) - 10;
    (*roach)++;
}

void publisher_update(info_t *move, char ch, void *publisher) {
    display_t display_info;
    display_info.pos_x = move->pos_x;
    display_info.pos_y = move->pos_y;
    display_info.ch = ch;
    display_info.score = move->points;
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

void draw_lizard(void *publisher, info_t *lizard, info_t lizard_data[], WINDOW *board, int delete) {
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

    wmove(board, aux.pos_x, aux.pos_y);
    waddch(board, head | A_BOLD);
    publisher_update(&aux, head, publisher);

    for (int i = 0; i < 5; i++) {
        new_position(&aux, aux.direction);
        if (lizard_here('?', aux.pos_x, aux.pos_y, lizard_data)) {
            continue;
        }
        wmove(board, aux.pos_x, aux.pos_y);
        waddch(board, aux.id | A_BOLD);
        publisher_update(&aux, aux.id, publisher);
    }
}

void draw_roach(void *publisher, info_t *roach, WINDOW *board, int delete) {
    char head = delete ? ' ' : roach->id;
    wmove(board, roach->pos_x, roach->pos_y);
    waddch(board, head | A_BOLD);
    publisher_update(roach, head, publisher);
}

int max(int a, int b) {
    return a > b ? a : b;
}

void move_lizard(info_t *move, direction_t direction, info_t lizard_data[], info_t roach_data[], int roaches) {
    info_t aux;
    int points;
    memcpy(&aux, move, sizeof(info_t));
    new_position(&aux, direction);
    for (int i = 0; i < LIZARDS_NUMBER; i++) {
        if (lizard_data[i].id != '0' && lizard_data[i].pos_x == aux.pos_x && lizard_data[i].pos_y == aux.pos_y) {
            points = (lizard_data[i].points + aux.points) / 2;
            lizard_data[i].points = points;
            move->points = points;
            move->direction = direction;
            return;
        }
    }
    for (int i = 0; i < roaches; i++) {
        if (roach_data[i].pos_x == aux.pos_x && roach_data[i].pos_y == aux.pos_y) {
            move->points += atoi(&roach_data[i].id);
            roach_data[i].eaten = time(NULL);
            roach_data[i].pos_x = -1;
            roach_data[i].pos_y = -1;
        }
    }
    new_position(move, direction);
}

void move_roach(info_t *move, direction_t direction, info_t lizard_data[]) {
    info_t aux;
    move->direction = direction;
    memcpy(&aux, move, sizeof(info_t));
    new_position(&aux, aux.direction);
    for (int i = 0; i < LIZARDS_NUMBER; i++) {
        if (lizard_data[i].id != '0' && lizard_data[i].pos_x == aux.pos_x && lizard_data[i].pos_y == aux.pos_y) {
            return;
        }
    }
    new_position(move, aux.direction);
}

int main(int argc, char *argv[]) {
    if (argc != 4) {
        printf("Usage: %s <req_port> <pub_port> <lib_path>\n", argv[0]);
        return 1;
    }

    char *endpoint;
    display_t display_info;
    msg_t msg;
    reply_t reply;
    info_t lizard_data[LIZARDS_NUMBER], roach_data[ROACHES_NUMBER], *move;
    int roaches = 0, roach, lizard, score;

    srand(time(NULL));

    for(int i = 0; i < LIZARDS_NUMBER; i++) {
        lizard_data[i].id = '0';
    }

    endpoint = (char*) malloc((max(strlen(argv[1]), strlen(argv[2])) + 8) * sizeof(char));
    // open server sockets
	void *context = zmq_ctx_new ();
    void *responder = zmq_socket (context, ZMQ_REP);
    sprintf(endpoint, "tcp://*:%s", argv[1]);
    zmq_bind (responder, endpoint);

    void *publisher = zmq_socket (context, ZMQ_PUB);
    sprintf(endpoint, "tcp://*:%s", argv[2]);
    zmq_bind (publisher, endpoint);

	initscr();
	cbreak();
    curs_set(0);
    keypad(stdscr, TRUE);
	noecho();

    // creates a window and draws a border
    WINDOW * board = newwin(WINDOW_SIZE + 2, WINDOW_SIZE + 2, 0, 0);
    box(board, 0 , 0);
	wrefresh(board);

    // prints the score
    WINDOW * score_board = newwin(WINDOW_SIZE + 2, 20, 0, WINDOW_SIZE + 3);
    box(score_board, 0 , 0);
    mvwprintw(score_board, 0, 5, "  Scores  ");
    wrefresh(score_board);
    
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
            init_lizard(move, lizard_data, lizard, &msg);
            draw_lizard(publisher, move, lizard_data, board, 0);
            reply.id = move->id; reply.password = move->password;
            mvwprintw(score_board, move->id - 'a' + 2, 11, "    ");
            wrefresh(score_board);
            mvwprintw(score_board, move->id - 'a' + 2, 1, "Lizard %c: %d", move->id, move->points);
            break;
        case ROACH_CONNECT:
            if (roaches == ROACHES_NUMBER) {
                invalid_msg(responder);
                continue;
            }
            move = &roach_data[roaches];
            init_roach(move, lizard_data, &roaches, &msg);
            draw_roach(publisher, move, board, 0);
            reply.id = move->id; reply.password = move->password;
            break;
        case LIZARD_MOVE:
            if (lizard_data[msg.id - 'a'].password != msg.password) {
                invalid_msg(responder);
                continue;
            }
            move = &lizard_data[msg.id - 'a'];
            draw_lizard(publisher, move, lizard_data, board, 1);
            move_lizard(move, msg.direction, lizard_data, roach_data, roaches);
            draw_lizard(publisher, move, lizard_data, board, 0);
            mvwprintw(score_board, move->id - 'a' + 2, 11, "    ");
            wrefresh(score_board);
            mvwprintw(score_board, move->id - 'a' + 2, 11, "%d", move->points);
            break;
        case ROACH_MOVE:
            roach = find_roach(roach_data, roaches, msg);
            if (roach == -1) {
                invalid_msg(responder);
                continue;
            }
            move = &roach_data[roach];
            if (move->pos_x != -1) {
                draw_roach(publisher, move, board, 1);
            }
            else if (move->pos_x == -1 && time(NULL) - move->eaten > 5) {
                move->pos_x = rand() % WINDOW_SIZE + 1;
                move->pos_y = rand() % WINDOW_SIZE + 1;
            }
            else {
                invalid_msg(responder);
                continue;
            }
            move_roach(move, msg.direction, lizard_data);
            draw_roach(publisher, move, board, 0);
            break;
        case LIZARD_DISCONNECT:
            if (lizard_data[msg.id - 'a'].password == msg.password) {
                move = &lizard_data[msg.id - 'a'];
                draw_lizard(publisher, move, lizard_data, board, 1);
                lizard_data[msg.id - 'a'].id = '0';
            }
            break;
        case DISPLAY_CONNECT:
            // TODO!
            continue;
        }
        wrefresh(score_board);
        wrefresh(board);
        reply.score = move->points;
        zmq_send(responder, &reply, sizeof(reply_t), 0);
    }
  	endwin();

    zmq_close (responder);
    zmq_close (publisher);
    zmq_ctx_destroy (context);

	return 0;
}