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

/**
 * @brief calculates new position and update it based on current position and direction
 * 
 * @param move lizard or roach data
 * @param direction direction to move
 */
void new_position(info_t *move, direction_t direction){
    move->direction = direction;

    // do not allow to move out of the board
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

/**
 * @brief checks if there is a lizard in given position
 * 
 * @param ch lizard id to ignore
 * @param pos_x position x
 * @param pos_y position y
 * @param lizard_data array of lizard data
 * @return int 1 if there is a lizard in given position, 0 otherwise
 */
int lizard_here(char ch, int pos_x, int pos_y, info_t lizard_data[]) {
    for(int i = 0; i < LIZARDS_NUMBER; i++) {
        // valid lizards do not have id '0'
        // ignoring the lizard that is moving
        if(lizard_data[i].id != '0' && lizard_data[i].id != ch && lizard_data[i].pos_x == pos_x && lizard_data[i].pos_y == pos_y) {
            return 1;
        }
    }
    return 0;
}

/**
 * @brief generates lizard data
 * 
 * @param lizard pointer to lizard
 * @param lizard_data lizard data array
 * @param id lizard id
 */
void init_lizard(info_t *lizard, info_t lizard_data[], int id) {
    lizard->id = id + 'a';
    lizard->password = rand();
    lizard->points = 0;

    // find free position
    do
    {
        lizard->pos_x = rand() % WINDOW_SIZE + 1;
        lizard->pos_y = rand() % WINDOW_SIZE + 1;
    } while (lizard_here(lizard->id, lizard->pos_x, lizard->pos_y, lizard_data));
    
    // set random direction
    lizard->direction = rand() % 4;
}

/**
 * @brief generates roach data
 * 
 * @param roach_data pointer to roach
 * @param lizard_data lizard data array
 * @param roach pointer to number of roaches
 * @param msg message with roach id
 */
void init_roach(info_t *roach_data, info_t lizard_data[], int *roach, msg_t *msg) {
    roach_data->id = msg->id;
    roach_data->password = rand();
    roach_data->points = atoi(&msg->id);

    // find free position
    do
    {
        roach_data->pos_x = rand() % WINDOW_SIZE + 1;
        roach_data->pos_y = rand() % WINDOW_SIZE + 1;
    } while (lizard_here(roach_data->id, roach_data->pos_x, roach_data->pos_y, lizard_data));
    
    // roach is not dead
    roach_data->eaten = time(NULL) - 10;
    (*roach)++;
}

/**
 * @brief updates display
 * 
 * @param move lizard or roach data
 * @param ch character to display
 * @param publisher socket to publish board updates
 */
void publisher_update(info_t *move, char ch, void *publisher) {
    // send display msg
    display_t display_info;
    display_info.pos_x = move->pos_x;
    display_info.pos_y = move->pos_y;
    display_info.ch = ch;
    display_info.score = move->points;
    assert(zmq_send(publisher, &display_info, sizeof(display_t), 0) == sizeof(display_t));
}

/**
 * @brief find roach with given password
 * 
 * @param roach_data roach data array
 * @param roaches number of roaches
 * @param msg message with password
 * @return int roach index (-1 if not found)
 */
int find_roach(info_t roach_data[], int roaches, msg_t msg) {
    for(int i = 0; i < roaches; i++) {
        if(roach_data[i].password == msg.password) {
            return i;
        }
    }
    return -1;
}

/**
 * @brief get first available lizard id
 * 
 * @param lizard_data lizard data array
 * @return int first available lizard id (-1 if full)
 */
int find_lizard(info_t lizard_data[]) {
    for(int i = 0; i < LIZARDS_NUMBER; i++) {
        if(lizard_data[i].id == '0') {
            return i;
        }
    }
    return -1;
}

/**
 * @brief reply to invalid messages
 * 
 * @param responder socket to reply
 */
void invalid_msg(void *responder) {
    reply_t reply;
    // guarentee that reply do not share private data
    reply.id = rand() % 256;
    reply.password = -1;
    assert(zmq_send(responder, &reply, sizeof(reply_t), 0) == sizeof(reply_t));
}

/**
 * @brief draws lizard in given position
 * 
 * @param publisher socket to publish board updates
 * @param lizard lizard to draw
 * @param lizard_data array of lizard data
 * @param board board window
 * @param delete boolean (1 to delete lizard, 0 to draw lizard)
 */
void draw_lizard(void *publisher, info_t *lizard, info_t lizard_data[], WINDOW *board, int delete) {
    info_t aux;
    memcpy(&aux, lizard, sizeof(info_t));
    char head = delete ? ' ' : lizard->id;

    // draw direction
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
    
    // lizard body
    if (delete) {
        aux.id = ' ';
    }
    else if (aux.points < 50) {
        aux.id = '.';
    }
    else {
        aux.id = '*';
    }

    // draw lizard head
    wmove(board, aux.pos_x, aux.pos_y);
    waddch(board, head | A_BOLD);
    publisher_update(&aux, head, publisher);

    // draw lizard body
    for (int i = 0; i < 5; i++) {
        // step to next position
        new_position(&aux, aux.direction);

        // do not delete other lizards
        if (lizard_here('?', aux.pos_x, aux.pos_y, lizard_data)) {
            continue;
        }
        wmove(board, aux.pos_x, aux.pos_y);
        waddch(board, aux.id | A_BOLD);

        // update display
        publisher_update(&aux, aux.id, publisher);
    }
}

/**
 * @brief draws roach in given position
 * 
 * @param publisher socket to publish board updates
 * @param roach roach to draw
 * @param board board window
 * @param delete boolean (1 to delete roach, 0 to draw roach)
 */
void draw_roach(void *publisher, info_t *roach, WINDOW *board, int delete) {
    char head = delete ? ' ' : roach->id;
    wmove(board, roach->pos_x, roach->pos_y);
    waddch(board, head | A_BOLD);
    publisher_update(roach, head, publisher);
}

/**
 * @brief calculates the maximum between two integers
 * 
 * @param a integer
 * @param b integer
 * @return int maximum between a and b
 */
int max(int a, int b) {
    return a > b ? a : b;
}

/**
 * @brief move lizard in given direction if valid (no lizard in the way)
 *        and update points
 * 
 * @param move pointer to lizard data
 * @param direction direction to move
 * @param lizard_data array of lizard data
 * @param roach_data array of roach data
 * @param roaches number of roaches
 */
void move_lizard(info_t *move, direction_t direction, info_t lizard_data[], info_t roach_data[], int roaches) {
    info_t aux;
    int points;
    memcpy(&aux, move, sizeof(info_t));

    // check if lizard is in the way
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

    // add points and kill roaches
    for (int i = 0; i < roaches; i++) {
        if (roach_data[i].pos_x == aux.pos_x && roach_data[i].pos_y == aux.pos_y) {
            move->points += atoi(&roach_data[i].id);
            roach_data[i].eaten = time(NULL);
            roach_data[i].pos_x = -1;
            roach_data[i].pos_y = -1;
        }
    }

    // move lizard
    new_position(move, direction);
}

/**
 * @brief move roach in given direction if valid (no lizard in the way)
 * 
 * @param move pointer to roach data
 * @param direction direction to move
 * @param lizard_data array of lizard data
 */
void move_roach(info_t *move, direction_t direction, info_t lizard_data[]) {
    info_t aux;
    move->direction = direction;
    memcpy(&aux, move, sizeof(info_t));

    // check if lizard is in the way
    new_position(&aux, aux.direction);
    for (int i = 0; i < LIZARDS_NUMBER; i++) {
        if (lizard_data[i].id != '0' && lizard_data[i].pos_x == aux.pos_x && lizard_data[i].pos_y == aux.pos_y) {
            return;
        }
    }

    // move roach
    new_position(move, aux.direction);
}

/**
 * @brief receives and processes messages from clients
 * 
 * @param argc 
 * @param argv replier and publisher ports
 * @return int error code (0 == success)
 */
int main(int argc, char *argv[]) {
    // argument check
    if (argc != 3) {
        printf("Usage: %s <req_port> <pub_port>\n", argv[0]);
        return 1;
    }

    char *endpoint;
    display_t display_info;
    msg_t msg;
    reply_t reply;
    info_t lizard_data[LIZARDS_NUMBER], roach_data[ROACHES_NUMBER], *move;
    int roaches = 0, roach, lizard, score;

    // initialize random seed
    srand(time(NULL));

    // initialize lizard data
    for(int i = 0; i < LIZARDS_NUMBER; i++) {
        lizard_data[i].id = '0';
    }

    // open server sockets
    endpoint = (char*) malloc((max(strlen(argv[1]), strlen(argv[2])) + 8) * sizeof(char));
    if (endpoint == NULL) {
        printf("Error allocating memory\n");
        return 1;
    }
	void *context = zmq_ctx_new ();
    assert(context != NULL);
    void *responder = zmq_socket (context, ZMQ_REP);
    assert(responder != NULL);
    sprintf(endpoint, "tcp://*:%s", argv[1]);
    assert(zmq_bind(responder, endpoint) == 0);

    void *publisher = zmq_socket (context, ZMQ_PUB);
    assert(publisher != NULL);
    sprintf(endpoint, "tcp://*:%s", argv[2]);
    assert(zmq_bind(publisher, endpoint) == 0);

    // initialize ncurses
	initscr();
	cbreak();
    curs_set(0);
    keypad(stdscr, TRUE);
	noecho();

    // board window
    WINDOW *board = newwin(WINDOW_SIZE + 2, WINDOW_SIZE + 2, 0, 0);
    assert(board != NULL);
    box(board, 0 , 0);
	wrefresh(board);

    // score board window
    WINDOW *score_board = newwin(WINDOW_SIZE + 2, 20, 0, WINDOW_SIZE + 3);
    assert(score_board != NULL);
    box(score_board, 0 , 0);
    mvwprintw(score_board, 0, 5, "  Scores  ");
    wrefresh(score_board);
    
    while (1)
    {
        assert(zmq_recv(responder, &msg, sizeof(msg_t), 0) == sizeof(msg_t));
        switch (msg.type) {
        case LIZARD_CONNECT:
            // new lizard will get first available id
            if ((lizard = find_lizard(lizard_data)) == -1) {
                invalid_msg(responder);
                continue;
            }
            move = &lizard_data[lizard];

            // generate lizard data
            init_lizard(move, lizard_data, lizard);

            // draw lizard
            draw_lizard(publisher, move, lizard_data, board, 0);
            reply.id = move->id; reply.password = move->password;

            // update score board
            mvwprintw(score_board, move->id - 'a' + 2, 11, "    ");
            wrefresh(score_board);
            mvwprintw(score_board, move->id - 'a' + 2, 1, "Lizard %c: %d", move->id, move->points);
            break;
        case ROACH_CONNECT:
            // server full
            if (roaches == ROACHES_NUMBER) {
                invalid_msg(responder);
                continue;
            }
            move = &roach_data[roaches];

            // generate roach data
            init_roach(move, lizard_data, &roaches, &msg);

            // draw roach
            draw_roach(publisher, move, board, 0);
            reply.id = move->id; reply.password = move->password;
            break;
        case LIZARD_MOVE:
            // validate lizard
            if (lizard_data[msg.id - 'a'].password != msg.password) {
                invalid_msg(responder);
                continue;
            }
            move = &lizard_data[msg.id - 'a'];

            // delete previous position
            draw_lizard(publisher, move, lizard_data, board, 1);

            // move lizard
            move_lizard(move, msg.direction, lizard_data, roach_data, roaches);

            // draw new position
            draw_lizard(publisher, move, lizard_data, board, 0);

            // update score board
            mvwprintw(score_board, move->id - 'a' + 2, 11, "    ");
            wrefresh(score_board);
            mvwprintw(score_board, move->id - 'a' + 2, 11, "%d", move->points);
            break;
        case ROACH_MOVE:
            // validate roach
            roach = find_roach(roach_data, roaches, msg);
            if (roach == -1) {
                invalid_msg(responder);
                continue;
            }
            move = &roach_data[roach];

            if (move->pos_x != -1) {
                // delete previous position
                draw_roach(publisher, move, board, 1);
            }
            else if (move->pos_x == -1 && time(NULL) - move->eaten > 5) {
                // respawn roach
                move->pos_x = rand() % WINDOW_SIZE + 1;
                move->pos_y = rand() % WINDOW_SIZE + 1;
            }
            else {
                invalid_msg(responder);
                continue;
            }

            // move roach
            move_roach(move, msg.direction, lizard_data);

            // draw new position
            draw_roach(publisher, move, board, 0);
            break;
        case LIZARD_DISCONNECT:
            // validate lizard
            if (lizard_data[msg.id - 'a'].password == msg.password) {
                move = &lizard_data[msg.id - 'a'];

                // delete previous position
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

        // reply to client
        assert(zmq_send(responder, &reply, sizeof(reply_t), 0) == sizeof(reply_t));
    }
  	endwin();

    zmq_close (responder);
    zmq_close (publisher);
    zmq_ctx_destroy (context);

	return 0;
}