#include <ncurses.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <dlfcn.h>
#include <assert.h>
#include <string.h>
#include <time.h>

#include "../lar-defs.h"
#include "../snd_rcv_proto.h"

#define LIZARDS_NUMBER 26
#define ROACHES_NUMBER WINDOW_SIZE *WINDOW_SIZE / 3

#define INVALID_MSG(socket, send_msg)                       \
    do                                                      \
    {                                                       \
        send_msg.success = 0;                               \
        send_msg.has_score = 0;                             \
        send_msg.has_password = 0;                          \
        send_msg.id = NULL;                                 \
        size_t packed_size;                                 \
        void *buffer;                                       \
        PACK__REPLY_MESSAGE(send_msg, buffer, packed_size); \
        SEND__MESSAGE(socket, buffer, packed_size);         \
    } while (0);

typedef struct info_t
{
    char id[2];
    int password;
    int points;
    int pos_x, pos_y;
    Direction direction;
    time_t eaten;
} info_t;

/**
 * @brief calculates new position and update it based on current position and direction
 *
 * @param move lizard or roach data
 * @param direction direction to move
 */
void new_position(info_t *move, Direction direction)
{
    move->direction = direction;

    // do not allow to move out of the board
    switch (move->direction)
    {
    case DIRECTION__UP:
        (move->pos_x)--;
        if (move->pos_x == 0)
            move->pos_x = 1;
        break;
    case DIRECTION__DOWN:
        (move->pos_x)++;
        if (move->pos_x == WINDOW_SIZE + 1)
            move->pos_x = WINDOW_SIZE;
        break;
    case DIRECTION__LEFT:
        (move->pos_y)--;
        if (move->pos_y == 0)
            move->pos_y = 1;
        break;
    case DIRECTION__RIGHT:
        (move->pos_y)++;
        if (move->pos_y == WINDOW_SIZE + 1)
            move->pos_y = WINDOW_SIZE;
        break;
    default:
        break;
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
int lizard_here(char ch, int pos_x, int pos_y, info_t lizard_data[])
{
    for (int i = 0; i < LIZARDS_NUMBER; i++)
    {
        // valid lizards do not have id '0'
        // ignoring the lizard that is moving
        if (lizard_data[i].id[0] != '0' && lizard_data[i].id[0] != ch && lizard_data[i].pos_x == pos_x && lizard_data[i].pos_y == pos_y)
        {
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
void init_lizard(info_t *lizard, info_t lizard_data[], int id)
{
    lizard->id[0] = id + 'a';
    lizard->id[1] = '\0';
    lizard->password = rand();
    lizard->points = 0;

    // find free position
    do
    {
        lizard->pos_x = rand() % WINDOW_SIZE + 1;
        lizard->pos_y = rand() % WINDOW_SIZE + 1;
    } while (lizard_here(lizard->id[0], lizard->pos_x, lizard->pos_y, lizard_data));

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
void init_roach(info_t *roach_data, info_t lizard_data[], int *roach, RequestMessage *msg)
{
    roach_data->id[0] = msg->id[0];
    roach_data->id[1] = '\0';
    roach_data->password = rand();
    roach_data->points = atoi(msg->id);

    // find free position
    do
    {
        roach_data->pos_x = rand() % WINDOW_SIZE + 1;
        roach_data->pos_y = rand() % WINDOW_SIZE + 1;
    } while (lizard_here(roach_data->id[0], roach_data->pos_x, roach_data->pos_y, lizard_data));

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
void publisher_update(info_t *move, char ch, void *publisher)
{
    // send display msg
    DisplayUpdateMessage display_info = DISPLAY_UPDATE_MESSAGE__INIT;
    void *buffer;
    size_t packed_size;

    display_info.ch = strdup(&ch);
    if (ch >= 'a' && ch <= 'z')
    {
        display_info.has_score = 1;
        display_info.score = move->points;
    }
    else
    {
        display_info.has_score = 0;
    }
    display_info.pos_x = move->pos_x;
    display_info.pos_y = move->pos_y;

    PACK__DISPLAY_UPDATE_MESSAGE(display_info, buffer, packed_size);
    SEND__MESSAGE(publisher, buffer, packed_size);
}

/**
 * @brief find roach with given password
 *
 * @param roach_data roach data array
 * @param roaches number of roaches
 * @param msg message with password
 * @return int roach index (-1 if not found)
 */
int find_roach(info_t roach_data[], int roaches, RequestMessage *msg)
{
    for (int i = 0; i < roaches; i++)
    {
        if (roach_data[i].password == msg->password)
        {
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
int find_lizard(info_t lizard_data[])
{
    for (int i = 0; i < LIZARDS_NUMBER; i++)
    {
        if (lizard_data[i].id[0] == '0')
        {
            return i;
        }
    }
    return -1;
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
void draw_lizard(void *publisher, info_t *lizard, info_t lizard_data[], WINDOW *board, int delete)
{
    info_t aux;
    memcpy(&aux, lizard, sizeof(info_t));
    char head = delete ? ' ' : lizard->id[0];

    // draw direction
    switch (lizard->direction)
    {
    case DIRECTION__LEFT:
        aux.direction = DIRECTION__RIGHT;
        break;
    case DIRECTION__RIGHT:
        aux.direction = DIRECTION__LEFT;
        break;
    case DIRECTION__UP:
        aux.direction = DIRECTION__DOWN;
        break;
    default:
        aux.direction = DIRECTION__UP;
    }

    // lizard body
    if (delete || aux.points < 0)
    {
        aux.id[0] = ' ';
    }
    else if (aux.points < 50)
    {
        aux.id[0] = '.';
    }
    else
    {
        aux.id[0] = '*';
    }

    // draw lizard head
    wmove(board, aux.pos_x, aux.pos_y);
    waddch(board, head | A_BOLD);
    publisher_update(&aux, head, publisher);

    // draw lizard body
    for (int i = 0; i < 5; i++)
    {
        // step to next position
        new_position(&aux, aux.direction);

        // do not delete other lizards
        if (lizard_here('?', aux.pos_x, aux.pos_y, lizard_data))
        {
            continue;
        }
        wmove(board, aux.pos_x, aux.pos_y);
        waddch(board, aux.id[0] | A_BOLD);

        // update display
        publisher_update(&aux, aux.id[0], publisher);
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
void draw_roach(void *publisher, info_t *roach, WINDOW *board, int delete)
{
    char head = delete ? ' ' : roach->id[0];
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
int max(int a, int b)
{
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
void move_lizard(info_t *move, Direction direction, info_t lizard_data[], info_t roach_data[], int roaches)
{
    info_t aux;
    int points;
    memcpy(&aux, move, sizeof(info_t));

    // check if lizard is in the way
    new_position(&aux, direction);
    for (int i = 0; i < LIZARDS_NUMBER; i++)
    {
        if (lizard_data[i].id[0] != '0' && lizard_data[i].pos_x == aux.pos_x && lizard_data[i].pos_y == aux.pos_y)
        {
            points = (lizard_data[i].points + aux.points) / 2;
            lizard_data[i].points = points;
            move->points = points;
            move->direction = direction;
            return;
        }
    }

    // add points and kill roaches
    for (int i = 0; i < roaches; i++)
    {
        if (roach_data[i].pos_x == aux.pos_x && roach_data[i].pos_y == aux.pos_y)
        {
            move->points += atoi(roach_data[i].id);
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
void move_roach(info_t *move, Direction direction, info_t lizard_data[])
{
    info_t aux;
    move->direction = direction;
    memcpy(&aux, move, sizeof(info_t));

    // check if lizard is in the way
    new_position(&aux, aux.direction);
    for (int i = 0; i < LIZARDS_NUMBER; i++)
    {
        if (lizard_data[i].id[0] != '0' && lizard_data[i].pos_x == aux.pos_x && lizard_data[i].pos_y == aux.pos_y)
        {
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
int main(int argc, char *argv[])
{
    // argument check
    if (argc != 3)
    {
        printf("Usage: %s <req_port> <pub_port>\n", argv[0]);
        return 1;
    }

    char *endpoint;
    DisplayUpdateMessage display_msg = DISPLAY_UPDATE_MESSAGE__INIT;
    RequestMessage *recv_msg;
    ReplyMessage send_msg = REPLY_MESSAGE__INIT;
    info_t lizard_data[LIZARDS_NUMBER], roach_data[ROACHES_NUMBER], *move;
    Type msg_type;
    int roaches = 0, roach, lizard, score;
    void *buffer;
    size_t packed_size;

    // initialize random seed
    srand(time(NULL));

    // initialize lizard data
    for (int i = 0; i < LIZARDS_NUMBER; i++)
    {
        lizard_data[i].id[0] = '0';
    }

    // open server sockets
    endpoint = (char *)malloc((max(strlen(argv[1]), strlen(argv[2])) + 8) * sizeof(char));
    if (endpoint == NULL)
    {
        printf("Error allocating memory\n");
        return 1;
    }
    void *context = zmq_ctx_new();
    assert(context != NULL);
    void *responder = zmq_socket(context, ZMQ_REP);
    assert(responder != NULL);
    sprintf(endpoint, "tcp://*:%s", argv[1]);
    assert(zmq_bind(responder, endpoint) == 0);

    void *publisher = zmq_socket(context, ZMQ_PUB);
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
    box(board, 0, 0);
    wrefresh(board);

    // score board window
    WINDOW *score_board = newwin(WINDOW_SIZE + 2, 20, 0, WINDOW_SIZE + 3);
    assert(score_board != NULL);
    box(score_board, 0, 0);
    mvwprintw(score_board, 0, 5, "  Scores  ");
    wrefresh(score_board);

    while (1)
    {
        assert(zmq_recv(responder, &msg_type, sizeof(Type), 0) == sizeof(Type));
        switch (msg_type)
        {
        case TYPE__LIZARD_CONNECT:
            RECV_UNPACK__REQUEST_MESSAGE(responder, recv_msg);
            // new lizard will get first available id
            if ((lizard = find_lizard(lizard_data)) == -1)
            {
                INVALID_MSG(responder, send_msg);
                continue;
            }
            move = &lizard_data[lizard];

            // generate lizard data
            init_lizard(move, lizard_data, lizard);

            // draw lizard
            draw_lizard(publisher, move, lizard_data, board, 0);
            send_msg.has_password = 1;
            send_msg.has_score = 0;
            send_msg.success = 1;
            send_msg.id = strdup(move->id);
            send_msg.password = move->password;

            // update score board
            mvwprintw(score_board, move->id[0] - 'a' + 2, 11, "    ");
            wrefresh(score_board);
            mvwprintw(score_board, move->id[0] - 'a' + 2, 1, "Lizard %c: %d", move->id[0], move->points);
            break;
        case TYPE__ROACH_CONNECT:
            RECV_UNPACK__REQUEST_MESSAGE(responder, recv_msg);
            // server full
            if (roaches == ROACHES_NUMBER)
            {
                INVALID_MSG(responder, send_msg);
                continue;
            }
            move = &roach_data[roaches];

            // generate roach data
            init_roach(move, lizard_data, &roaches, recv_msg);

            // draw roach
            draw_roach(publisher, move, board, 0);

            send_msg.has_password = 1;
            send_msg.has_score = 0;
            send_msg.success = 1;
            send_msg.id = strdup(move->id);
            send_msg.password = move->password;
            break;
        case TYPE__LIZARD_MOVE:
            RECV_UNPACK__REQUEST_MESSAGE(responder, recv_msg);
            // validate lizard
            if (strlen(lizard_data->id) == 1 && lizard_data[recv_msg->id[0] - 'a'].password != recv_msg->password)
            {
                INVALID_MSG(responder, send_msg);
                continue;
            }
            move = &lizard_data[recv_msg->id[0] - 'a'];

            // delete previous position
            draw_lizard(publisher, move, lizard_data, board, 1);

            // move lizard
            move_lizard(move, recv_msg->direction, lizard_data, roach_data, roaches);

            // draw new position
            draw_lizard(publisher, move, lizard_data, board, 0);

            // update score board
            mvwprintw(score_board, move->id[0] - 'a' + 2, 11, "    ");
            wrefresh(score_board);
            mvwprintw(score_board, move->id[0] - 'a' + 2, 11, "%d", move->points);

            send_msg.has_password = 0;
            send_msg.has_score = 1;
            send_msg.success = 1;
            send_msg.score = move->points;
            send_msg.id = NULL;
            break;
        case TYPE__ROACH_MOVE:
            RECV_UNPACK__REQUEST_MESSAGE(responder, recv_msg);
            // validate roach
            roach = find_roach(roach_data, roaches, recv_msg);
            if (roach == -1)
            {
                INVALID_MSG(responder, send_msg);
                continue;
            }
            move = &roach_data[roach];

            if (move->pos_x != -1)
            {
                // delete previous position
                draw_roach(publisher, move, board, 1);
            }
            else if (move->pos_x == -1 && time(NULL) - move->eaten > 5)
            {
                // respawn roach
                move->pos_x = rand() % WINDOW_SIZE + 1;
                move->pos_y = rand() % WINDOW_SIZE + 1;
            }
            else
            {
                INVALID_MSG(responder, send_msg);
                continue;
            }

            // move roach
            move_roach(move, recv_msg->direction, lizard_data);

            // draw new position
            draw_roach(publisher, move, board, 0);

            send_msg.has_password = 0;
            send_msg.has_score = 0;
            send_msg.success = 1;
            send_msg.id = NULL;
            break;
        case TYPE__LIZARD_DISCONNECT:
            RECV_UNPACK__REQUEST_MESSAGE(responder, recv_msg);
            // validate lizard
            if (strlen(recv_msg->id) == 1 && lizard_data[recv_msg->id[0] - 'a'].password == recv_msg->password)
            {
                move = &lizard_data[recv_msg->id[0] - 'a'];

                // delete previous position
                draw_lizard(publisher, move, lizard_data, board, 1);
                lizard_data[recv_msg->id[0] - 'a'].id[0] = '0';
            }

            send_msg.has_password = 0;
            send_msg.has_score = 0;
            send_msg.success = 1;
            send_msg.id = NULL;

            break;
        default:
            INVALID_MSG(responder, send_msg);
            continue;
        }
        wrefresh(score_board);
        wrefresh(board);

        // reply to client
        PACK__REPLY_MESSAGE(send_msg, buffer, packed_size);
        SEND__MESSAGE(responder, buffer, packed_size);
    }
    endwin();

    zmq_close(responder);
    zmq_close(publisher);
    zmq_ctx_destroy(context);

    return 0;
}