#include "roaches-lib.h"
#include "lar-defs.h"
#include "lizard-lib.h"

#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h> // include time

#define ROACHES_NUMBER WINDOW_SIZE *WINDOW_SIZE / 3

static info_t roach_data[ROACHES_NUMBER];
static int roaches = 0;

static pthread_mutex_t roach_data_lock[ROACHES_NUMBER] = {PTHREAD_MUTEX_INITIALIZER};

/**
 * @brief Respawns roach in a random position after 5 seconds
 *
 * @param roach_ pointer to roach struct (return of find_roach)
 */
void *respawn_roach(void *arg)
{
    long int i = (long int)arg;
    info_t *roach = &roach_data[i];
    // respawn after 5 seconds in a random position
    sleep(5);
    pthread_mutex_lock(&roach_data_lock[i]);
    roach->pos_x = rand() % WINDOW_SIZE + 1;
    roach->pos_y = rand() % WINDOW_SIZE + 1;
    pthread_mutex_unlock(&roach_data_lock[i]);
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
        pthread_mutex_lock(&roach_data_lock[i]);
        roach_data[i].id[0] = '9';
        roach_data[i].id[1] = '\0';
        pthread_mutex_unlock(&roach_data_lock[i]);
    }
}

/**
 * @brief generates roach data
 *
 * @param i index of roach
 * @param msg message with roach id
 */
void init_roach(int i, RequestMessage *msg)
{
    info_t *roach = &roach_data[i];
    pthread_mutex_lock(&roach_data_lock[i]);
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

    pthread_mutex_unlock(&roach_data_lock[i]);
    roaches++;
}

/**
 * @brief find roach with given password
 *
 * @param msg message with password
 * @return int index of roach, -1 if not found
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
 * @param i roach position in roach_data (obtained with find_roach)
 * @param board board window
 * @param delete boolean (1 to delete roach, 0 to draw roach)
 */
void draw_roach(void *publisher, int i, WINDOW *board, int delete)
{
    info_t *roach = &roach_data[i];
    char head = delete ? ' ' : roach->id[0];
    wmove(board, roach->pos_x, roach->pos_y);
    waddch(board, head | A_BOLD);
    publisher_update(roach, head, publisher);
}

/**
 * @brief move roach in given direction if valid (no lizard in the way)
 *
 * @param m roach position in roach_data (obtained with find_roach)
 * @param direction direction to move
 */
void move_roach(int m, Direction direction)
{
    info_t *move = &roach_data[m];
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
    pthread_mutex_lock(&roach_data_lock[m]);
    new_position(move, aux.direction);
    pthread_mutex_unlock(&roach_data_lock[m]);
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
    for (long int i = 0; i < roaches; i++)
    {
        if (roach_data[i].pos_x == pos_x && roach_data[i].pos_y == pos_y)
        {
            points += atoi(roach_data[i].id);
            pthread_mutex_lock(&roach_data_lock[i]);
            roach_data[i].pos_x = -1;
            roach_data[i].pos_y = -1;
            pthread_mutex_unlock(&roach_data_lock[i]);
            pthread_t time_5_sec;
            pthread_create(&time_5_sec, NULL, respawn_roach, (void *)i);
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
 * @return int index of free roach, -1 if none is free
 */
int get_next_free_roach()
{
    for (int i = 0; i < ROACHES_NUMBER; i++)
    {
        if (roach_data[i].id[0] == '9')
        {
            return i;
        }
    }
    return -1;
}

/**
 * @brief Fills reply message with roach data
 *
 * @param move position of roach in roach_data (obtained with find_roach)
 * @param msg reply message to fill
 */
void fill_roach_data(int move, ReplyMessage *msg)
{
    fill_id_and_password(&roach_data[move], msg);
}

/**
 * @brief Checks if roach is dead
 *
 * @param i position of roach in roach_data (obtained with find_roach)
 * @return int 1 if roach is dead, 0 otherwise
 */
int roach_dead(int i)
{
    info_t *roach = &roach_data[i];
    return roach->pos_x == -1;
}
