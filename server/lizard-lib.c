#include "lizard-lib.h"
#include "lar-defs.h"
#include "bots-lib.h"

#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <time.h>
#include <unistd.h>

#define LIZARDS_NUMBER 26

static info_t lizard_data[LIZARDS_NUMBER];

static pthread_mutex_t lizard_data_lock[LIZARDS_NUMBER] = {PTHREAD_MUTEX_INITIALIZER};

/**
 * @brief initializes lizard data array
 * with the default value '0' that means
 * no lizard in that position
 */
void init_lizards()
{
    // initialize lizard data
    for (int i = 0; i < LIZARDS_NUMBER; i++)
    {
        pthread_mutex_lock(&lizard_data_lock[i]);
        lizard_data[i].id[0] = '0';
        pthread_mutex_unlock(&lizard_data_lock[i]);
    }
}

/**
 * @brief checks if there is a lizard in given position
 *
 * @param ch lizard id to ignore
 * @param pos_x position x
 * @param pos_y position y
 * @return int 1 if there is a lizard in given position, 0 otherwise
 */
int lizard_here(char ch, int pos_x, int pos_y)
{
    for (int i = 0; i < LIZARDS_NUMBER; i++)
    {
        // valid lizards do not have id '0'
        // ignoring the lizard that is moving
        if (lizard_data[i].id[0] != '0' && lizard_data[i].id[0] != ch && lizard_data[i].pos_x == pos_x && lizard_data[i].pos_y == pos_y)
        {
            return 1;
        }
    }
    return 0;
}

/**
 * @brief generates lizard data
 *
 * @param lizard_ pointer to lizard struct (return of get_lizard)
 * @param id lizard position in lizard data array
 */
void init_lizard(void *lizard_, int id)
{
    info_t *lizard = (info_t *)lizard_;

    pthread_mutex_lock(&lizard_data_lock[id]);
    lizard->id[0] = id + 'a';
    lizard->id[1] = '\0';
    lizard->password = rand();
    lizard->points = 0;
    lizard->eaten = time(NULL);

    // find free position
    do
    {
        lizard->pos_x = rand() % WINDOW_SIZE + 1;
        lizard->pos_y = rand() % WINDOW_SIZE + 1;
    } while (lizard_here(lizard->id[0], lizard->pos_x, lizard->pos_y) || wasp_here(lizard->pos_x, lizard->pos_y));

    // set random direction
    lizard->direction = rand() % 4;
    pthread_mutex_unlock(&lizard_data_lock[id]);
}

/**
 * @brief get first available lizard id
 *
 * @return int first available lizard position (-1 if full)
 */
int find_lizard()
{
    for (int i = 0; i < LIZARDS_NUMBER; i++)
    {
        if (lizard_data[i].id[0] == '0')
        {
            return i;
        }
    }
    return -1;
}

/**
 * @brief draws lizard in given position
 *
 * @param publisher socket to publish board updates
 * @param lizard pointer to lizard struct (return of get_lizard)
 * @param board board window
 * @param delete boolean (1 to delete lizard, 0 to draw lizard)
 */
void draw_lizard(void *publisher, void *lizard_, WINDOW *board, int delete)
{
    info_t *lizard = (info_t *)lizard_;
    info_t aux;
    memcpy(&aux, lizard, sizeof(info_t));
    char head = delete ? ' ' : lizard->id[0];

    // draw direction
    switch (lizard->direction)
    {
    case DIRECTION__LEFT:
        aux.direction = DIRECTION__RIGHT;
        break;
    case DIRECTION__RIGHT:
        aux.direction = DIRECTION__LEFT;
        break;
    case DIRECTION__UP:
        aux.direction = DIRECTION__DOWN;
        break;
    default:
        aux.direction = DIRECTION__UP;
    }

    // lizard body
    if (delete)
    {
        aux.id[0] = ' ';
    }
    else if (aux.points < 50)
    {
        aux.id[0] = '.';
    }
    else
    {
        aux.id[0] = '*';
    }

    // draw lizard head
    wmove(board, aux.pos_x, aux.pos_y);
    waddch(board, head | A_BOLD);
    publisher_update(&aux, head, publisher);

    if (lizard->points < 0)
    {
        return;
    }

    // draw lizard body
    for (int i = 0; i < 5; i++)
    {
        // step to next position
        new_position(&aux, aux.direction);

        // do not delete other lizards
        if (lizard_here('?', aux.pos_x, aux.pos_y))
        {
            continue;
        }
        wmove(board, aux.pos_x, aux.pos_y);
        waddch(board, aux.id[0] | A_BOLD);

        // update display
        publisher_update(&aux, aux.id[0], publisher);
    }
}

/**
 * @brief move lizard in given direction if valid (no lizard in the way)
 *        and update points
 *
 * @param move_ pointer to lizard struct (return of get_lizard)
 * @param direction direction to move
 */
void move_lizard(void *move_, Direction direction)
{
    info_t *move = (info_t *)move_;
    move->eaten = time(NULL);
    info_t aux;
    int points;
    int id = move->id[0] - 'a';
    memcpy(&aux, move, sizeof(info_t));

    // check if lizard is in the way
    new_position(&aux, direction);
    if (wasp_here(aux.pos_x, aux.pos_y))
    {
        move->points -= 10;
        move->direction = direction;
        return;
    }

    for (int i = 0; i < LIZARDS_NUMBER; i++)
    {
        if (lizard_data[i].id[0] != '0' && lizard_data[i].pos_x == aux.pos_x && lizard_data[i].pos_y == aux.pos_y)
        {
            points = (lizard_data[i].points + aux.points) / 2;
            pthread_mutex_lock(&lizard_data_lock[i]);
            lizard_data[i].points = points;
            pthread_mutex_unlock(&lizard_data_lock[i]);
            pthread_mutex_lock(&lizard_data_lock[id]);
            move->points = points;
            move->direction = direction;
            pthread_mutex_unlock(&lizard_data_lock[id]);
            return;
        }
    }

    // add points and kill roaches
    move->points += kill_roaches(aux.pos_x, aux.pos_y);

    // move lizard
    pthread_mutex_lock(&lizard_data_lock[id]);
    new_position(move, direction);
    pthread_mutex_unlock(&lizard_data_lock[id]);
}

/**
 * @brief Get pointer to lizard
 *
 * @param id lizard position in lizard data array
 */
void *get_lizard(int id)
{
    return &lizard_data[id];
}

/**
 * @brief Fill reply message with lizard data
 *
 * @param move pointer to lizard struct (return of get_lizard)
 * @param send_msg reply message to fill
 */
void fill_lizard_data(void *move, ReplyMessage *send_msg)
{
    fill_id_and_password((info_t *)move, send_msg);
    send_msg->has_score = 1;
    send_msg->score = ((info_t *)move)->points;
}

/**
 * @brief Validate message information
 *
 * @param recv_msg message to validate
 * @return int 1 if valid, 0 otherwise
 */
int valid_lizard(RequestMessage *recv_msg)
{
    for (int i = 0; i < LIZARDS_NUMBER; i++)
    {
        if (lizard_data[i].id[0] != '0' && strcmp(lizard_data[i].id, recv_msg->id) == 0 && lizard_data[i].password == recv_msg->password)
        {
            return 1;
        }
    }
    return 0;
}

/**
 * @brief delete lizard
 *
 * @param lizard_ pointer to lizard struct (return of get_lizard)
 * @return void* pointer to lizard id
 */
void delete_lizard(void *lizard_)
{
    info_t *lizard = (info_t *)lizard_;
    int id = lizard->id[0] - 'a';
    pthread_mutex_lock(&lizard_data_lock[id]);
    lizard->id[0] = '0';
    pthread_mutex_unlock(&lizard_data_lock[id]);
}

/**
 * @brief Finds a lizard in a given position and stungs it
 * if there is one
 *
 * @param pos_x position x to stung
 * @param pos_y position y to stung
 * @return int 1 if stung, 0 otherwise
 */
int stung_lizard(int pos_x, int pos_y)
{
    for (int i = 0; i < LIZARDS_NUMBER; i++)
    {
        if (lizard_data[i].id[0] != '0' && lizard_data[i].pos_x == pos_x && lizard_data[i].pos_y == pos_y)
        {
            pthread_mutex_lock(&lizard_data_lock[i]);
            lizard_data[i].points -= 10;
            pthread_mutex_unlock(&lizard_data_lock[i]);
            return 1;
        }
    }
    return 0;
}

/**
 * @brief Disconnects lizards offline
 *
 * @param arg publisher socket
 * @return no return
 */
void *offline_lizards(void *arg)
{
    void *publisher = ((void **)arg)[0];
    WINDOW *board = ((WINDOW **)arg)[1];
    while (1)
    {
        sleep(3);
        for (int i = 0; i < LIZARDS_NUMBER; i++)
        {
            if (lizard_data[i].id[0] != '0' && time(NULL) - lizard_data[i].eaten > 60)
            {
                pthread_mutex_lock(&lizard_data_lock[i]);
                lizard_data[i].id[0] = '0';
                pthread_mutex_unlock(&lizard_data_lock[i]);
                draw_lizard(publisher, &lizard_data[i], board, 1);
                wrefresh(board);
            }
        }
    }
    return (void *)NULL;
}
