#ifndef __ROACHES_LIB_H__
#define __ROACHES_LIB_H__

#include <ncurses.h>
#include "../lar-defs.pb-c.h"

#define WINDOW_SIZE 30

void init_roaches();
void init_roach(void *roach_, RequestMessage *msg);
void *find_roach(RequestMessage *msg);
void draw_roach(void *publisher, void *roach, WINDOW *board, int delete);
void move_roach(void *move_, Direction direction);
int kill_roaches(int pos_x, int pos_y);
int roaches_full();
void *get_next_free_roach();
void fill_roach_data(void *move_, ReplyMessage *msg);
int roach_dead(void *roach_);

#endif // __ROACHES_LIB_H__