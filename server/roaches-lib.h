#ifndef __ROACHES_LIB_H__
#define __ROACHES_LIB_H__

#include <ncurses.h>
#include "../lar-defs.pb-c.h"

#define WINDOW_SIZE 30

void init_roaches();
void init_roach(int i, RequestMessage *msg);
int find_roach(RequestMessage *msg);
void draw_roach(void *publisher, int i, WINDOW *board, int delete);
void move_roach(int m, Direction direction);
int kill_roaches(int pos_x, int pos_y);
int roaches_full();
int get_next_free_roach();
void fill_roach_data(int move, ReplyMessage *msg);
int roach_dead(int i);

#endif // __ROACHES_LIB_H__