#ifndef __LAR_DEFS_H__
#define __LAR_DEFS_H__

#include <time.h>
#include "../lar-defs.pb-c.h"

#define WINDOW_SIZE 30

typedef struct info_t
{
    char id[2];
    int password;
    int points;
    int pos_x, pos_y;
    Direction direction;
    time_t eaten;
} info_t;

void publisher_update(info_t *move, char ch, void *publisher);
void new_position(info_t *move, Direction direction);

#endif // __LAR_DEFS_H__