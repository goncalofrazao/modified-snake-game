#ifndef __ROACHES_LIB_H__
#define __ROACHES_LIB_H__

#include <ncurses.h>
#include "../lar-defs.pb-c.h"

#define WINDOW_SIZE 30

void init_bots();
void init_bot(int i, RequestMessage *msg);
int find_bot(RequestMessage *msg);
void draw_bot(void *publisher, int i, WINDOW *board, int delete);
int wasp_here(int pos_x, int pos_y);
void move_bot(int m, Direction direction);
int kill_roaches(int pos_x, int pos_y);
int bots_full();
int get_next_free_bot();
void fill_bot_data(int move, ReplyMessage *msg);
int roach_dead(int i);
void delete_bot(int i);

#endif // __ROACHES_LIB_H__