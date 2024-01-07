#ifndef __ROACHES_LIB_H__
#define __ROACHES_LIB_H__

#include <ncurses.h>
#include "../lar-defs.pb-c.h"

#define WINDOW_SIZE 30

void init_roach(RequestMessage *msg);
int find_roach(RequestMessage *msg);
void draw_roach(void *publisher, void *roach, WINDOW *board, int delete);
void move_roach(void *move_, Direction direction);
int kill_roaches(int pos_x, int pos_y);

#endif // __ROACHES_LIB_H__