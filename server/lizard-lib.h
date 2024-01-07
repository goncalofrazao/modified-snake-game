#ifndef __LIZARD_LIB_H__
#define __LIZARD_LIB_H__

#include <ncurses.h>
#include "../lar-defs.pb-c.h"

void init_lizards();
int lizard_here(char ch, int pos_x, int pos_y);
void init_lizard(void *lizard, int id);
int find_lizard();
void draw_lizard(void *publisher, void *lizard_, WINDOW *board, int delete);
void move_lizard(void *move_, Direction direction);
void *get_lizard(int id);
void *get_lizard_id(void *lizard_);
void fill_lizard_data(void *move, ReplyMessage *send_msg);
int valid_lizard(RequestMessage *recv_msg);
void delete_lizard(void *lizard_);
int stung_lizard(int pos_x, int pos_y);
void *offline_lizards(void *arg);

#endif // __LIZARD_LIB_H__