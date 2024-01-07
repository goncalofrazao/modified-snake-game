#include "bots-lib.h"
#include "lar-defs.h"
#include "lizard-lib.h"

#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>

#define BOTS_NUMBER WINDOW_SIZE *WINDOW_SIZE / 3

static info_t bot_data[BOTS_NUMBER];

/**
 * @brief Respawns roach in a random position after 5 seconds
 *
 * @param roach_ pointer to roach struct (return of find_roach)
 */
void *respawn_roach(void *arg)
{
    long int i = (long int)arg;
    info_t *roach = &bot_data[i];
    // respawn after 5 seconds in a random position
    sleep(5);
    do
    {
        roach->pos_x = rand() % WINDOW_SIZE + 1;
        roach->pos_y = rand() % WINDOW_SIZE + 1;
    } while (lizard_here(roach->id[0], roach->pos_x, roach->pos_y) || wasp_here(roach->pos_x, roach->pos_y));
    return (void *)roach;
}

/**
 * @brief initializes bot data
 * initializes all ids to '9' (invalid id)
 * valid ids are between '1' and '5'
 *
 */
void init_bots()
{
    for (int i = 0; i < BOTS_NUMBER; i++)
    {
        bot_data[i].id[0] = '9';
        bot_data[i].id[1] = '\0';
    }
}

/**
 * @brief Checks if there is a roach in given position
 *
 * @param pos_x position x
 * @param pos_y position y
 * @return int 1 if there is a wasp, 0 otherwise
 */
int roach_here(int pos_x, int pos_y)
{
    for (int i = 0; i < BOTS_NUMBER; i++)
    {
        if (bot_data[i].id[0] != '9' && bot_data[i].id[0] != '#' && bot_data[i].pos_x == pos_x && bot_data[i].pos_y == pos_y)
        {
            return 1;
        }
    }
    return 0;
}

/**
 * @brief generates bot data
 *
 * @param i index of bot
 * @param msg message with bot id
 */
void init_bot(int i, RequestMessage *msg)
{
    info_t *bot = &bot_data[i];
    bot->id[0] = msg->id[0];
    bot->id[1] = '\0';
    bot->password = rand();
    bot->points = atoi(msg->id);

    // find free position
    do
    {
        bot->pos_x = rand() % WINDOW_SIZE + 1;
        bot->pos_y = rand() % WINDOW_SIZE + 1;
    } while (lizard_here(bot->id[0], bot->pos_x, bot->pos_y) || (bot->id[0] != '#' && wasp_here(bot->pos_x, bot->pos_y)) || (bot->id[0] == '#' && roach_here(bot->pos_x, bot->pos_y) && wasp_here(bot->pos_x, bot->pos_y) > 1));
}

/**
 * @brief find bot with given password
 *
 * @param msg message with password
 * @return int index of bot, -1 if not found
 */
int find_bot(RequestMessage *msg)
{
    for (int i = 0; i < BOTS_NUMBER; i++)
    {
        if (bot_data[i].id[0] == msg->id[0] && bot_data[i].password == msg->password)
        {
            return i;
        }
    }
    return -1;
}

/**
 * @brief draws bot in given position
 *
 * @param publisher socket to publish board updates
 * @param i bot position in bot_data (obtained with find_bot)
 * @param board board window
 * @param delete boolean (1 to delete bot, 0 to draw bot)
 */
void draw_bot(void *publisher, int i, WINDOW *board, int delete)
{
    info_t *bot = &bot_data[i];
    char head = delete ? ' ' : bot->id[0];
    wmove(board, bot->pos_x, bot->pos_y);
    waddch(board, head | A_BOLD);
    publisher_update(bot, head, publisher);
}

/**
 * @brief Checks if there is a wasp in given position
 *
 * @param pos_x position x
 * @param pos_y position y
 * @return int 1 if there is a wasp, 0 otherwise
 */
int wasp_here(int pos_x, int pos_y)
{
    int n = 0;
    for (int i = 0; i < BOTS_NUMBER; i++)
    {
        if (bot_data[i].id[0] == '#' && bot_data[i].pos_x == pos_x && bot_data[i].pos_y == pos_y)
        {
            n++;
        }
    }
    return n;
}

/**
 * @brief move bot in given direction if valid (no lizard in the way)
 *
 * @param m bot position in bot_data (obtained with find_bot)
 * @param direction direction to move
 */
void move_bot(int m, Direction direction)
{
    info_t *move = &bot_data[m];
    info_t aux;
    move->direction = direction;
    memcpy(&aux, move, sizeof(info_t));

    // check if lizard is in the way
    new_position(&aux, aux.direction);
    switch (move->id[0])
    {
    case '#':
        if (stung_lizard(aux.pos_x, aux.pos_y) || wasp_here(aux.pos_x, aux.pos_y) || roach_here(aux.pos_x, aux.pos_y))
        {
            return;
        }
        break;

    default:
        if (lizard_here('?', aux.pos_x, aux.pos_y) || wasp_here(aux.pos_x, aux.pos_y))
        {
            return;
        }
        break;
    }

    // move bot
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
    for (long int i = 0; i < BOTS_NUMBER; i++)
    {
        if (bot_data[i].id[0] != '9' && bot_data[i].id[0] != '#' && bot_data[i].pos_x == pos_x && bot_data[i].pos_y == pos_y)
        {
            points += atoi(bot_data[i].id);
            bot_data[i].pos_x = -1;
            bot_data[i].pos_y = -1;
            pthread_t time_5_sec;
            pthread_create(&time_5_sec, NULL, respawn_roach, (void *)i);
        }
    }

    return points;
}

/**
 * @brief Gets a free bot struct
 *
 * @return int index of free bot, -1 if none is free
 */
int get_next_free_bot()
{
    for (int i = 0; i < BOTS_NUMBER; i++)
    {
        if (bot_data[i].id[0] == '9')
        {
            return i;
        }
    }
    return -1;
}

/**
 * @brief Fills reply message with bot data
 *
 * @param move position of bot in bot_data (obtained with find_bot)
 * @param msg reply message to fill
 */
void fill_bot_data(int move, ReplyMessage *msg)
{
    fill_id_and_password(&bot_data[move], msg);
}

/**
 * @brief Checks if roach is dead
 *
 * @param i position of roach in bot_data (obtained with find_roach)
 * @return int 1 if roach is dead, 0 otherwise
 */
int roach_dead(int i)
{
    info_t *roach = &bot_data[i];
    return roach->pos_x == -1;
}

/**
 * @brief Deletes bot
 *
 * @param i position of bot in bot_data (obtained with find_bot)
 */
void delete_bot(int i)
{
    bot_data[i].id[0] = '9';
}
