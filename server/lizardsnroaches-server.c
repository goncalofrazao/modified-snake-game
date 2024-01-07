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
#include "bots-lib.h"
#include "lizard-lib.h"
#include "thread-manager.h"

#define MAX(a, b) ((a) > (b) ? (a) : (b))

#define LIZARDS_THREADS 5
#define BOTS_THREADS 5

/**
 * @brief receives and processes messages from clients
 * alocates a multithreaded server of a snake game
 * with 3 different types of clients: lizards, roaches and wasps.
 *
 * @param argc
 * @param argv replier and publisher ports
 * @return int error code (0 == success)
 */
int main(int argc, char *argv[])
{
    // argument check
    if (argc != 4)
    {
        printf("Usage: %s <lizards_port> <roaches_port> <screen_port>\n", argv[0]);
        return 1;
    }

    char *endpoint;
    int rc;

    // initialize random seed
    srand(time(NULL));

    // initialize lizards and roaches
    init_lizards();
    init_bots();

    // open server sockets
    endpoint = (char *)malloc((MAX(strlen(argv[1]), strlen(argv[2])) + 8) * sizeof(char));
    if (endpoint == NULL)
    {
        printf("Error allocating memory\n");
        return 1;
    }

    void *context = zmq_ctx_new();
    assert(context != NULL);

    // lizard sockets
    sprintf(endpoint, "tcp://*:%s", argv[1]);
    ProxyManager lizard_proxy_manager;
    lizard_proxy_manager.frontend = zmq_socket(context, ZMQ_ROUTER);
    assert(lizard_proxy_manager.frontend != NULL);
    rc = zmq_bind(lizard_proxy_manager.frontend, endpoint);
    assert(rc == 0);
    lizard_proxy_manager.backend = zmq_socket(context, ZMQ_DEALER);
    assert(lizard_proxy_manager.backend != NULL);
    rc = zmq_bind(lizard_proxy_manager.backend, "inproc://lizard-back-end");
    assert(rc == 0);

    // bots sockets
    sprintf(endpoint, "tcp://*:%s", argv[2]);
    ProxyManager bots_proxy_manager;
    bots_proxy_manager.frontend = zmq_socket(context, ZMQ_ROUTER);
    assert(bots_proxy_manager.frontend != NULL);
    rc = zmq_bind(bots_proxy_manager.frontend, endpoint);
    assert(rc == 0);
    bots_proxy_manager.backend = zmq_socket(context, ZMQ_DEALER);
    assert(bots_proxy_manager.backend != NULL);
    rc = zmq_bind(bots_proxy_manager.backend, "inproc://bots-back-end");
    assert(rc == 0);

    // screen sockets
    sprintf(endpoint, "tcp://*:%s", argv[3]);
    void *publisher = zmq_socket(context, ZMQ_PUB);
    assert(publisher != NULL);
    rc = zmq_bind(publisher, endpoint);
    assert(rc == 0);

    // initialize ncurses
    initscr();
    clear();
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

    ThreadManager *thread_manager = init_thread_manager(context, board, score_board, publisher);

    void **offline_lizard_args = (void **)malloc(2 * sizeof(void *));
    if (offline_lizard_args == NULL)
    {
        printf("Error allocating memory\n");
        return 1;
    }
    offline_lizard_args[0] = (void *)publisher;
    offline_lizard_args[1] = (void *)board;
    pthread_t offline_lizard_thread;
    pthread_create(&offline_lizard_thread, NULL, offline_lizards, (void *)offline_lizard_args);

    // create lizard threads
    pthread_t lizard_threads[LIZARDS_THREADS];
    for (int i = 0; i < LIZARDS_THREADS; i++)
    {
        pthread_create(&lizard_threads[i], NULL, lizard_handle, (void *)thread_manager);
    }

    // create roach threads
    pthread_t roach_threads[BOTS_THREADS];
    for (int i = 0; i < BOTS_THREADS; i++)
    {
        pthread_create(&roach_threads[i], NULL, bots_handle, (void *)thread_manager);
    }

    // create proxy threads
    pthread_t lizard_proxy_thread;
    pthread_create(&lizard_proxy_thread, NULL, run_proxy, (void *)&lizard_proxy_manager);
    pthread_t roach_proxy_thread;
    pthread_create(&roach_proxy_thread, NULL, run_proxy, (void *)&bots_proxy_manager);

    // wait for threads to finish
    pthread_join(lizard_proxy_thread, NULL);

    // cleanup
    endwin();
    zmq_close(publisher);
    zmq_ctx_destroy(context);

    free(thread_manager);
    free(endpoint);

    return 0;
}