#include "thread-manager.h"
#include "../snd_rcv_proto.h"
#include "lizard-lib.h"
#include "bots-lib.h"

#include <zmq.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>

static pthread_mutex_t board_lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t score_board_lock = PTHREAD_MUTEX_INITIALIZER;

struct _thread_manager
{
    void *context;
    WINDOW *board;
    WINDOW *score_board;
    void *publisher;
};

/**
 * @brief Updates score board
 *
 * @param score_board pointer to ncurses window
 * @param id position of lizard to update
 * @param score score to update
 */
void update_score_board(WINDOW *score_board, int id, int score)
{
    mvwprintw(score_board, id + 2, 1, "Lizard %c:         ", id + 'a');
    wrefresh(score_board);
    mvwprintw(score_board, id + 2, 11, "%d", score);
    wrefresh(score_board);
}

/**
 * @brief Inits thread struct manager with the parameters needed in the threads
 *
 * @param context zmq context
 * @param board ncurses window
 * @param score_board ncurses window
 * @param publisher zmq publisher
 * @return ThreadManager* pointer to thread manager struct
 */
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

/**
 * @brief Function to run a proxy in a thread
 *
 * @param arg ProxyManager struct with frontend and backend sockets
 * @return void* the proxy manager struct
 */
void *run_proxy(void *arg)
{
    ProxyManager *proxy_manager = (ProxyManager *)arg;
    zmq_proxy(proxy_manager->frontend, proxy_manager->backend, NULL);
    return (void *)proxy_manager;
}

/**
 * @brief Thread to handle lizard requests
 *
 * @param arg pointer to ThreadManager struct with zmq sockets and ncurses windows
 * @return void*
 */
void *lizard_handle(void *arg)
{
    // extract information from thread manager struct
    void *context = ((ThreadManager *)arg)->context;
    WINDOW *board = ((ThreadManager *)arg)->board;
    WINDOW *score_board = ((ThreadManager *)arg)->score_board;
    void *publisher = ((ThreadManager *)arg)->publisher;

    // connect to backend socket
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
        // message type comes separetad from the rest of the message
        rc = zmq_recv(responder, &msg_type, sizeof(Type), 0);
        if (rc == -1)
        {
            printf("Error in zmq_socket: %s\n", zmq_strerror(zmq_errno()));
            exit(1);
        }

        switch (msg_type)
        {
        case TYPE__LIZARD_CONNECT:
            // macro to unpack message
            RECV_UNPACK__REQUEST_MESSAGE(responder, recv_msg);

            // get available lizard id (-1 if full)
            id = find_lizard();
            if (id == -1)
            {
                INVALID_MSG(responder, send_msg);
                continue;
            }

            // move lizard to new position
            move = get_lizard(id);
            pthread_mutex_lock(&board_lock);
            init_lizard(move, id);
            draw_lizard(publisher, move, board, 0);
            wrefresh(board);
            pthread_mutex_unlock(&board_lock);

            // fill reply message
            send_msg.success = 1;
            send_msg.has_score = 0;
            send_msg.has_password = 1;
            fill_lizard_data(move, &send_msg);

            // update score board
            pthread_mutex_lock(&score_board_lock);
            update_score_board(score_board, id, send_msg.score);
            pthread_mutex_unlock(&score_board_lock);
            break;

        case TYPE__LIZARD_MOVE:
            // unpack message
            RECV_UNPACK__REQUEST_MESSAGE(responder, recv_msg);

            // validate lizard data
            if (recv_msg->id == NULL || !valid_lizard(recv_msg))
            {
                INVALID_MSG(responder, send_msg);
                continue;
            }

            id = recv_msg->id[0] - 'a';
            move = get_lizard(id);

            pthread_mutex_lock(&board_lock);

            // move lizard
            // delete previous position and draw new position
            draw_lizard(publisher, move, board, 1);
            move_lizard(move, recv_msg->direction);
            draw_lizard(publisher, move, board, 0);
            wrefresh(board);
            pthread_mutex_unlock(&board_lock);

            // fill reply message
            send_msg.success = 1;
            send_msg.has_score = 1;
            send_msg.has_password = 0;
            fill_lizard_data(move, &send_msg);

            // update score board
            pthread_mutex_lock(&score_board_lock);
            update_score_board(score_board, id, send_msg.score);
            pthread_mutex_unlock(&score_board_lock);
            break;

        case TYPE__LIZARD_DISCONNECT:
            // unpack message
            RECV_UNPACK__REQUEST_MESSAGE(responder, recv_msg);

            // validate lizard data
            if (recv_msg->id == NULL || !valid_lizard(recv_msg))
            {
                INVALID_MSG(responder, send_msg);
                continue;
            }

            // get lizard id
            id = recv_msg->id[0] - 'a';
            move = get_lizard(id);

            // delete previous position
            pthread_mutex_lock(&board_lock);
            draw_lizard(publisher, move, board, 1);
            wrefresh(board);
            pthread_mutex_unlock(&board_lock);

            // delete lizard data
            delete_lizard(move);

            // fill reply message
            send_msg.success = 1;
            send_msg.has_password = 0;
            send_msg.has_score = 0;
            send_msg.id = NULL;
            break;

        default:
            continue;
        }
        // pack and send reply message
        PACK__REPLY_MESSAGE(send_msg, buffer, packed_size);
        SEND__MESSAGE(responder, buffer, packed_size);
    }

    zmq_close(responder);
}

/**
 * @brief Thread to handle wasps and roaches requests
 *
 * @param arg pointer to ThreadManager struct with zmq sockets and ncurses windows
 * @return void* no return
 */
void *bots_handle(void *arg)
{
    // extract information from thread manager struct
    void *context = ((ThreadManager *)arg)->context;
    WINDOW *board = ((ThreadManager *)arg)->board;
    WINDOW *score_board = ((ThreadManager *)arg)->score_board;
    void *publisher = ((ThreadManager *)arg)->publisher;

    // connect to backend socket
    void *responder = zmq_socket(context, ZMQ_REP);
    assert(responder != NULL);
    int rc = zmq_connect(responder, "inproc://bots-back-end");

    RequestMessage *recv_msg;
    ReplyMessage send_msg = REPLY_MESSAGE__INIT;
    DisplayUpdateMessage display_msg = DISPLAY_UPDATE_MESSAGE__INIT;
    Type msg_type;
    int id;

    void *buffer;
    size_t packed_size;

    while (1)
    {
        // message type comes separetad from the rest of the message
        rc = zmq_recv(responder, &msg_type, sizeof(Type), 0);
        if (rc == -1)
        {
            printf("Error in zmq_socket: %s\n", zmq_strerror(zmq_errno()));
            exit(1);
        }

        switch (msg_type)
        {
        case TYPE__BOT_CONNECT:
            // macro to unpack message
            RECV_UNPACK__REQUEST_MESSAGE(responder, recv_msg);

            // get available roach id (-1 if full)
            id = get_next_free_bot();
            if (id == -1 || recv_msg->id == NULL || strlen(recv_msg->id) != 1)
            {
                INVALID_MSG(responder, send_msg);
                continue;
            }

            // initialize bot information
            // draw bot
            pthread_mutex_lock(&board_lock);
            init_bot(id, recv_msg);
            draw_bot(publisher, id, board, 0);
            wrefresh(board);
            pthread_mutex_unlock(&board_lock);

            // fill reply message
            send_msg.success = 1;
            send_msg.has_score = 0;
            send_msg.has_password = 1;
            fill_bot_data(id, &send_msg);
            break;

        case TYPE__BOT_MOVE:
            // macro to unpack message
            RECV_UNPACK__REQUEST_MESSAGE(responder, recv_msg);

            // get bot id from message (returns -1 if not found)
            id = find_bot(recv_msg);
            if (id == -1 || roach_dead(id))
            {
                INVALID_MSG(responder, send_msg);
                continue;
            }

            // move bot
            // delete previous position and draw new position
            pthread_mutex_lock(&board_lock);
            draw_bot(publisher, id, board, 1);
            move_bot(id, recv_msg->direction);
            draw_bot(publisher, id, board, 0);
            wrefresh(board);
            pthread_mutex_unlock(&board_lock);

            // fill reply message
            send_msg.success = 1;
            send_msg.has_score = 0;
            send_msg.has_password = 0;
            send_msg.id = NULL;
            break;

        case TYPE__BOT_DISCONNECT:
            // macro to unpack message
            RECV_UNPACK__REQUEST_MESSAGE(responder, recv_msg);

            // get bot id from message (returns -1 if not found)
            id = find_bot(recv_msg);
            if (id == -1)
            {
                printf("Invalid roach\n");
                INVALID_MSG(responder, send_msg);
                continue;
            }

            // delete previous position
            pthread_mutex_lock(&board_lock);
            draw_bot(publisher, id, board, 1);
            wrefresh(board);
            pthread_mutex_unlock(&board_lock);

            // delete bot data
            delete_bot(id);

            // fill reply message
            send_msg.success = 1;
            send_msg.has_score = 0;
            send_msg.has_password = 0;
            send_msg.id = NULL;
            break;
        default:
            continue;
        }

        // pack and send reply message
        PACK__REPLY_MESSAGE(send_msg, buffer, packed_size);
        SEND__MESSAGE(responder, buffer, packed_size);
    }

    return NULL;
}
