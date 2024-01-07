#include "roaches_lib.h"
#include "lar-defs.h"
#include "lizard_lib.h"

#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h> // include time

#define ROACHES_NUMBER WINDOW_SIZE *WINDOW_SIZE / 3

static info_t roach_data[ROACHES_NUMBER];
static int roaches = 0;

static pthread_mutex_t roach_data_lock = PTHREAD_MUTEX_INITIALIZER;

/**
 * @brief Respawns roach in a random position after 5 seconds
 *
 * @param roach_ pointer to roach struct (return of find_roach)
 */
void *respawn_roach(void *roach_)
{
    info_t *roach = (info_t *)roach_;
    // respawn after 5 seconds in a random position
    sleep(5);
    pthread_mutex_lock(&roach_data_lock);
    roach->pos_x = rand() % WINDOW_SIZE + 1;
    roach->pos_y = rand() % WINDOW_SIZE + 1;
    pthread_mutex_unlock(&roach_data_lock);
    return (void *)roach;
}

/**
 * @brief initializes roach data
 * initializes all ids to '9' (invalid id)
 * valid ids are between '1' and '5'
 *
 */
void init_roaches()
{
    for (int i = 0; i < ROACHES_NUMBER; i++)
    {
        roach_data[i].id[0] = '9';
        roach_data[i].id[1] = '\0';
    }
}

/**
 * @brief generates roach data
 *
 * @param msg message with roach id
 */
void init_roach(void *roach_, RequestMessage *msg)
{
    info_t *roach = (info_t *)roach_;
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
 * @param msg message with password
 * @return void* pointer to roach struct
 */
void *find_roach(RequestMessage *msg)
{
    for (int i = 0; i < roaches; i++)
    {
        if (roach_data[i].password == msg->password)
        {
            return &roach_data[i];
        }
    }
    return NULL;
}

/**
 * @brief draws roach in given position
 *
 * @param publisher socket to publish board updates
 * @param roach_ pointer to roach struct
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

/**
 * @brief Killes all roaches in given position
 *
 * @param pos_x x position
 * @param pos_y y position
 * @return int sum of points of killed roaches
 */
int kill_roaches(int pos_x, int pos_y)
{
    int points = 0;
    // add points and kill roaches
    for (int i = 0; i < roaches; i++)
    {
        if (roach_data[i].pos_x == pos_x && roach_data[i].pos_y == pos_y)
        {
            points += atoi(roach_data[i].id);
            pthread_mutex_lock(&roach_data_lock);
            roach_data[i].pos_x = -1;
            roach_data[i].pos_y = -1;
            pthread_mutex_unlock(&roach_data_lock);
            pthread_t time_5_sec;
            pthread_create(&time_5_sec, NULL, respawn_roach, (void *)&roach_data[i]);
        }
    }

    return points;
}

/**
 * @brief Checks if there is no space for more roaches
 *
 * @return int 1 if there is no space, 0 otherwise
 */
int roaches_full()
{
    return roaches == ROACHES_NUMBER;
}

/**
 * @brief Gets a free roach struct
 *
 * @return void* pointer to roach struct
 */
void *get_next_free_roach()
{
    for (int i = 0; i < ROACHES_NUMBER; i++)
    {
        if (roach_data[i].id[0] == '9')
        {
            return &roach_data[i];
        }
    }
    return NULL;
}

/**
 * @brief Fills reply message with roach data
 *
 * @param move pointer to roach struct (return of find_roach)
 * @param msg reply message to fill
 */
void fill_roach_data(void *move_, ReplyMessage *msg)
{
    fill_id_and_password((info_t *)move_, msg);
}

/**
 * @brief Checks if roach is dead
 *
 * @param roach_ pointer to roach struct (return of find_roach)
 * @return int 1 if roach is dead, 0 otherwise
 */
int roach_dead(void *roach_)
{
    info_t *roach = (info_t *)roach_;
    return roach->pos_x == -1;
}
