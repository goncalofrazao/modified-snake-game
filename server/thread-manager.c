#include "thread-manager.h"
#include "../snd_rcv_proto.h"
#include "lizard-lib.h"
#include "roaches-lib.h"

#include <zmq.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>

static pthread_mutex_t board_lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t score_board_lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t move_lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t lizard_connection_lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t roach_connection_lock = PTHREAD_MUTEX_INITIALIZER;

struct _thread_manager
{
    void *context;
    WINDOW *board;
    WINDOW *score_board;
    void *publisher;
};

void update_score_board(WINDOW *score_board, int id, int score)
{
    mvwprintw(score_board, id + 2, 11, "    ");
    wrefresh(score_board);
    mvwprintw(score_board, id + 2, 11, "%d", score);
    wrefresh(score_board);
}

ThreadManager *init_thread_manager(void *context, WINDOW *board, WINDOW *score_board, void *publisher)
{
    ThreadManager *thread_manager = (ThreadManager *)malloc(sizeof(ThreadManager));
    if (thread_manager == NULL)
    {
        printf("Error allocating memory\n");
        exit(1);
    }
    thread_manager->context = context;
    thread_manager->board = board;
    thread_manager->score_board = score_board;
    thread_manager->publisher = publisher;
    return thread_manager;
}

void *run_proxy(void *arg)
{
    ProxyManager *proxy_manager = (ProxyManager *)arg;
    zmq_proxy(proxy_manager->frontend, proxy_manager->backend, NULL);
    return (void *)proxy_manager;
}

void *lizard_handle(void *arg)
{
    void *context = ((ThreadManager *)arg)->context;
    WINDOW *board = ((ThreadManager *)arg)->board;
    WINDOW *score_board = ((ThreadManager *)arg)->score_board;
    void *publisher = ((ThreadManager *)arg)->publisher;

    void *responder = zmq_socket(context, ZMQ_REP);
    assert(responder != NULL);
    int rc = zmq_connect(responder, "inproc://lizard-back-end");
    assert(rc == 0);

    RequestMessage *recv_msg;
    ReplyMessage send_msg = REPLY_MESSAGE__INIT;
    DisplayUpdateMessage display_msg = DISPLAY_UPDATE_MESSAGE__INIT;
    Type msg_type;
    void *move;
    int id;

    void *buffer;
    size_t packed_size;

    while (1)
    {
        rc = zmq_recv(responder, &msg_type, sizeof(Type), 0);
        if (rc == -1)
        {
            printf("Error in zmq_socket: %s\n", zmq_strerror(zmq_errno()));
            exit(1);
        }

        switch (msg_type)
        {
        case TYPE__LIZARD_CONNECT:
            RECV_UNPACK__REQUEST_MESSAGE(responder, recv_msg);

            // check if lizard is already connected
            pthread_mutex_lock(&lizard_connection_lock);
            id = find_lizard();
            if (id == -1)
            {
                INVALID_MSG(responder, send_msg);
            }
            move = get_lizard(id);

            pthread_mutex_lock(&move_lock);
            init_lizard(move, id);
            pthread_mutex_unlock(&move_lock);
            pthread_mutex_unlock(&lizard_connection_lock);

            pthread_mutex_lock(&board_lock);
            draw_lizard(publisher, move, board, 0);
            wrefresh(board);
            pthread_mutex_unlock(&board_lock);

            send_msg.success = 1;
            send_msg.has_score = 0;
            send_msg.has_password = 1;
            fill_lizard_data(move, &send_msg);

            pthread_mutex_lock(&score_board_lock);
            update_score_board(score_board, id, send_msg.score);
            pthread_mutex_unlock(&score_board_lock);
            break;

        case TYPE__LIZARD_MOVE:
            RECV_UNPACK__REQUEST_MESSAGE(responder, recv_msg);
            // validate lizard
            if (recv_msg->id == NULL || !valid_lizard(recv_msg))
            {
                INVALID_MSG(responder, send_msg);
                continue;
            }
            id = recv_msg->id[0] - 'a';
            move = get_lizard(id);

            pthread_mutex_lock(&move_lock);
            move_lizard(move, recv_msg->direction);
            pthread_mutex_unlock(&move_lock);

            pthread_mutex_lock(&board_lock);
            draw_lizard(publisher, move, board, 1);
            draw_lizard(publisher, move, board, 0);
            wrefresh(board);
            pthread_mutex_unlock(&board_lock);

            send_msg.success = 1;
            send_msg.has_score = 1;
            send_msg.has_password = 0;
            fill_lizard_data(move, &send_msg);

            pthread_mutex_lock(&score_board_lock);
            update_score_board(score_board, id, send_msg.score);
            pthread_mutex_unlock(&score_board_lock);
            break;

        case TYPE__LIZARD_DISCONNECT:
            RECV_UNPACK__REQUEST_MESSAGE(responder, recv_msg);
            // validate lizard
            if (recv_msg->id == NULL || !valid_lizard(recv_msg))
            {
                id = recv_msg->id[0] - 'a';
                move = get_lizard(id);

                pthread_mutex_lock(&board_lock);
                draw_lizard(publisher, move, board, 1);
                wrefresh(board);
                pthread_mutex_unlock(&board_lock);

                // delete previous position
                pthread_mutes_lock(&lizard_connection_lock);
                pthread_mutex_lock(&move_lock);
                delete_lizard(move);
                pthread_mutex_unlock(&lizard_connection_lock);
                pthread_mutex_unlock(&move_lock);
            }

            send_msg.success = 1;
            send_msg.has_password = 0;
            send_msg.has_score = 0;
            send_msg.id = NULL;
            break;

        default:
            continue;
        }
        PACK__REPLY_MESSAGE(send_msg, buffer, packed_size);
        SEND__MESSAGE(responder, buffer, packed_size);
    }

    zmq_close(responder);
}

void *roach_handle(void *arg)
{
    void *context = ((ThreadManager *)arg)->context;
    WINDOW *board = ((ThreadManager *)arg)->board;
    WINDOW *score_board = ((ThreadManager *)arg)->score_board;
    void *publisher = ((ThreadManager *)arg)->publisher;

    void *responder = zmq_socket(context, ZMQ_REP);
    assert(responder != NULL);
    int rc = zmq_connect(responder, "inproc://roaches-back-end");

    RequestMessage *recv_msg;
    ReplyMessage send_msg = REPLY_MESSAGE__INIT;
    DisplayUpdateMessage display_msg = DISPLAY_UPDATE_MESSAGE__INIT;
    Type msg_type;
    int id;

    void *buffer;
    size_t packed_size;

    while (1)
    {
        rc = zmq_recv(responder, &msg_type, sizeof(Type), 0);
        if (rc == -1)
        {
            printf("Error in zmq_socket: %s\n", zmq_strerror(zmq_errno()));
            exit(1);
        }

        switch (msg_type)
        {
        case TYPE__ROACH_CONNECT:
            RECV_UNPACK__REQUEST_MESSAGE(responder, recv_msg);
            // roaches full
            if (roaches_full())
            {
                INVALID_MSG(responder, send_msg);
                continue;
            }
            pthread_mutex_lock(&roach_connection_lock);
            id = get_next_free_roach();
            pthread_mutex_lock(&move_lock);
            init_roach(id, recv_msg);
            pthread_mutex_unlock(&move_lock);
            pthread_mutex_unlock(&roach_connection_lock);

            pthread_mutex_lock(&board_lock);
            draw_roach(publisher, id, board, 0);
            wrefresh(board);
            pthread_mutex_unlock(&board_lock);

            send_msg.success = 1;
            send_msg.has_score = 0;
            send_msg.has_password = 1;
            fill_roach_data(id, &send_msg);
            break;

        case TYPE__ROACH_MOVE:
            RECV_UNPACK__REQUEST_MESSAGE(responder, recv_msg);
            // validate roach
            id = find_roach(recv_msg);
            pthread_mutex_lock(&move_lock);
            if (id == -1 || roach_dead(id))
            {
                INVALID_MSG(responder, send_msg);
                continue;
            }
            move_roach(id, recv_msg->direction);
            pthread_mutex_unlock(&move_lock);

            pthread_mutex_lock(&board_lock);
            draw_roach(publisher, id, board, 1);
            draw_roach(publisher, id, board, 0);
            wrefresh(board);
            pthread_mutex_unlock(&board_lock);

            send_msg.success = 1;
            send_msg.has_score = 0;
            send_msg.has_password = 0;
            send_msg.id = NULL;
            break;
        default:
            continue;
        }

        // reply to client
        PACK__REPLY_MESSAGE(send_msg, buffer, packed_size);
        SEND__MESSAGE(responder, buffer, packed_size);
    }
}

void *t_since_last_msg(void *arg)
{
}