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
#include <pthread.h>

#include "../snd_rcv_proto.h"
#include "roaches-lib.h"
#include "lizard-lib.h"

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

#define MAX(a, b) ((a) > (b) ? (a) : (b))

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
    void *move;
    int id;
    Type msg_type;
    void *buffer;
    size_t packed_size;

    // initialize random seed
    srand(time(NULL));

    init_lizards();
    init_roaches();

    // open server sockets
    endpoint = (char *)malloc((MAX(strlen(argv[1]), strlen(argv[2])) + 8) * sizeof(char));
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
            if ((id = find_lizard()) == -1)
            {
                INVALID_MSG(responder, send_msg);
                continue;
            }
            move = get_lizard(id);

            // generate lizard data
            init_lizard(move, id);

            // draw lizard
            draw_lizard(publisher, move, board, 0);
            send_msg.has_password = 1;
            send_msg.has_score = 0;
            send_msg.success = 1;
            fill_lizard_data(move, &send_msg);

            // update score board
            mvwprintw(score_board, send_msg.id[0] - 'a' + 2, 11, "    ");
            wrefresh(score_board);
            mvwprintw(score_board, send_msg.id[0] - 'a' + 2, 1, "Lizard %c: %d", send_msg.id[0], send_msg.score);
            break;
        case TYPE__ROACH_CONNECT:
            RECV_UNPACK__REQUEST_MESSAGE(responder, recv_msg);
            // server full
            if (roaches_full())
            {
                INVALID_MSG(responder, send_msg);
                continue;
            }
            id = get_next_free_roach();

            // generate roach data
            init_roach(id, recv_msg);

            // draw roach
            draw_roach(publisher, id, board, 0);

            send_msg.has_password = 1;
            send_msg.has_score = 0;
            send_msg.success = 1;
            fill_roach_data(id, &send_msg);
            break;
        case TYPE__LIZARD_MOVE:
            RECV_UNPACK__REQUEST_MESSAGE(responder, recv_msg);
            // validate lizard
            if (recv_msg->id == NULL || !valid_lizard(recv_msg))
            {
                INVALID_MSG(responder, send_msg);
                continue;
            }
            move = get_lizard(recv_msg->id[0] - 'a');

            // delete previous position
            draw_lizard(publisher, move, board, 1);

            // move lizard
            move_lizard(move, recv_msg->direction);

            // draw new position
            draw_lizard(publisher, move, board, 0);

            send_msg.has_password = 0;
            send_msg.has_score = 1;
            send_msg.success = 1;
            fill_lizard_data(move, &send_msg);

            // update score board
            mvwprintw(score_board, send_msg.id[0] - 'a' + 2, 11, "    ");
            wrefresh(score_board);
            mvwprintw(score_board, send_msg.id[0] - 'a' + 2, 11, "%d", send_msg.score);
            break;
        case TYPE__ROACH_MOVE:
            RECV_UNPACK__REQUEST_MESSAGE(responder, recv_msg);
            // validate roach
            id = find_roach(recv_msg);
            if (id == -1)
            {
                INVALID_MSG(responder, send_msg);
                continue;
            }

            if (roach_dead(id))
            {
                // delete previous position
                INVALID_MSG(responder, send_msg);
                continue;
            }

            // delete roach
            draw_roach(publisher, id, board, 1);

            // move roach
            move_roach(id, recv_msg->direction);

            // draw new position
            draw_roach(publisher, id, board, 0);

            send_msg.has_password = 0;
            send_msg.has_score = 0;
            send_msg.success = 1;
            send_msg.id = NULL;
            break;
        case TYPE__LIZARD_DISCONNECT:
            RECV_UNPACK__REQUEST_MESSAGE(responder, recv_msg);
            // validate lizard
            if (recv_msg->id == NULL || !valid_lizard(recv_msg))
            {
                move = get_lizard(recv_msg->id[0] - 'a');

                // delete previous position
                draw_lizard(publisher, move, board, 1);
                delete_lizard(move);
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