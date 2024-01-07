#include "roaches_lib.h"
#include "lar-defs.h"
#include "lizard_lib.h"

#include <time.h>
#include <stdlib.h>
#include <string.h>

#define ROACHES_NUMBER WINDOW_SIZE *WINDOW_SIZE / 3

static info_t roach_data[ROACHES_NUMBER];
static int roaches = 0;

/**
 * @brief generates roach data
 *
 * @param roach_data pointer to roach
 * @param lizard_data lizard data array
 * @param roach pointer to number of roaches
 * @param msg message with roach id
 */
void init_roach(RequestMessage *msg)
{
    info_t *roach = &roach_data[roaches];
    roach->id[0] = msg->id[0];
    roach->id[1] = '\0';
    roach->password = rand();
    roach->points = atoi(msg->id);

    // find free position
    do
    {
        roach->pos_x = rand() % WINDOW_SIZE + 1;
        roach->pos_y = rand() % WINDOW_SIZE + 1;
    } while (lizard_here(roach->id[0], roach->pos_x, roach->pos_y));

    // roach is not dead
    roach->eaten = time(NULL) - 10;
    roaches++;
}

/**
 * @brief find roach with given password
 *
 * @param roach_data roach data array
 * @param roaches number of roaches
 * @param msg message with password
 * @return int roach index (-1 if not found)
 */
int find_roach(RequestMessage *msg)
{
    for (int i = 0; i < roaches; i++)
    {
        if (roach_data[i].password == msg->password)
        {
            return i;
        }
    }
    return -1;
}

/**
 * @brief draws roach in given position
 *
 * @param publisher socket to publish board updates
 * @param roach roach to draw
 * @param board board window
 * @param delete boolean (1 to delete roach, 0 to draw roach)
 */
void draw_roach(void *publisher, void *roach_, WINDOW *board, int delete)
{
    info_t *roach = (info_t *)roach_;
    char head = delete ? ' ' : roach->id[0];
    wmove(board, roach->pos_x, roach->pos_y);
    waddch(board, head | A_BOLD);
    publisher_update(roach, head, publisher);
}

/**
 * @brief move roach in given direction if valid (no lizard in the way)
 *
 * @param move pointer to roach data
 * @param direction direction to move
 * @param lizard_data array of lizard data
 */
void move_roach(void *move_, Direction direction)
{
    info_t *move = (info_t *)move_;
    info_t aux;
    move->direction = direction;
    memcpy(&aux, move, sizeof(info_t));

    // check if lizard is in the way
    new_position(&aux, aux.direction);
    if (lizard_here('?', aux.pos_x, aux.pos_y))
    {
        return;
    }

    // move roach
    new_position(move, aux.direction);
}

int kill_roaches(int pos_x, int pos_y)
{
    int points = 0;
    // add points and kill roaches
    for (int i = 0; i < roaches; i++)
    {
        if (roach_data[i].pos_x == pos_x && roach_data[i].pos_y == pos_y)
        {
            points += atoi(roach_data[i].id);
            roach_data[i].eaten = time(NULL);
            roach_data[i].pos_x = -1;
            roach_data[i].pos_y = -1;
        }
    }

    return points;
}