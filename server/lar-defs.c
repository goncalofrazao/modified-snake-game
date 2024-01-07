#include "lar-defs.h"
#include "../lar-defs.pb-c.h"
#include "../snd_rcv_proto.h"

#include <stdlib.h>
#include <string.h>

/**
 * @brief updates display
 *
 * @param move lizard or roach data
 * @param ch character to display
 * @param publisher socket to publish board updates
 */
void publisher_update(info_t *move, char ch, void *publisher)
{
    // send display msg
    DisplayUpdateMessage display_info = DISPLAY_UPDATE_MESSAGE__INIT;
    void *buffer;
    size_t packed_size;

    display_info.ch = strdup(&ch);
    if (ch >= 'a' && ch <= 'z')
    {
        display_info.has_score = 1;
        display_info.score = move->points;
    }
    else
    {
        display_info.has_score = 0;
    }
    display_info.pos_x = move->pos_x;
    display_info.pos_y = move->pos_y;

    PACK__DISPLAY_UPDATE_MESSAGE(display_info, buffer, packed_size);
    SEND__MESSAGE(publisher, buffer, packed_size);
}

/**
 * @brief calculates new position and update it based on current position and direction
 *
 * @param move lizard or roach data
 * @param direction direction to move
 */
void new_position(info_t *move, Direction direction)
{
    move->direction = direction;

    // do not allow to move out of the board
    switch (move->direction)
    {
    case DIRECTION__UP:
        (move->pos_x)--;
        if (move->pos_x == 0)
            move->pos_x = 1;
        break;
    case DIRECTION__DOWN:
        (move->pos_x)++;
        if (move->pos_x == WINDOW_SIZE + 1)
            move->pos_x = WINDOW_SIZE;
        break;
    case DIRECTION__LEFT:
        (move->pos_y)--;
        if (move->pos_y == 0)
            move->pos_y = 1;
        break;
    case DIRECTION__RIGHT:
        (move->pos_y)++;
        if (move->pos_y == WINDOW_SIZE + 1)
            move->pos_y = WINDOW_SIZE;
        break;
    default:
        break;
    }
}