#ifndef __SERVER_THREAD_MANAGER_H__
#define __SERVER_THREAD_MANAGER_H__

#include <ncurses.h>

struct _proxy_manager
{
    void *frontend;
    void *backend;
};

typedef struct _thread_manager ThreadManager;
typedef struct _proxy_manager ProxyManager;

ThreadManager *init_thread_manager(void *context, WINDOW *board, WINDOW *score_board, void *publisher);
void *run_proxy(void *arg);
void *lizard_handle(void *arg);
void *roach_handle(void *arg);

#endif // __SERVER_THREAD_MANAGER_H__