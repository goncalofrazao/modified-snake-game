#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <assert.h>
#include <pthread.h>

#include "../snd_rcv_proto.h"

#define MAX_WASPS 10

static Type msgtype = TYPE__BOT_CONNECT;
static RequestMessage *wasps;
static ReplyMessage *reply;
static void *buffer;
static size_t packed_size;
static int id;
static int num_wasps;

/**
 * @brief
 *
 * @param arg requester socket
 * @return void*
 */
void *handle_wasp(void *arg)
{
    void *requester = (void *)arg;

    // randomly choose number of wasps
    srand(time(NULL));
    num_wasps = rand() % MAX_WASPS + 1;
    wasps = (RequestMessage *)malloc(num_wasps * sizeof(RequestMessage));

    // connect possible wasps to server
    for (int i = 0; i < num_wasps; i++)
    {
        // connect message
        zmq_send(requester, &msgtype, sizeof(Type), ZMQ_SNDMORE);
        request_message__init(&wasps[i]);
        wasps[i].id = (char *)malloc(2 * sizeof(char));
        wasps[i].id[0] = '#';
        wasps[i].id[1] = '\0';
        wasps[i].has_direction = 0;
        wasps[i].has_password = 0;
        PACK__REQUEST_MESSAGE(wasps[i], buffer, packed_size);
        SEND__MESSAGE(requester, buffer, packed_size);

        // receive password
        RECV_UNPACK__REPLY_MESSAGE(requester, reply);

        // save password
        wasps[i].password = reply->password;
        wasps[i].has_password = 1;

        // full field
        if (wasps[i].password == 0 && i != 0)
        {
            printf("Wasps - Field full!\n");
            num_wasps = i;
            printf("Wasps - %d wasps connected\n", num_wasps);
            wasps = (RequestMessage *)realloc(wasps, num_wasps * sizeof(RequestMessage));
            break;
        }
        else if (wasps[i].password == 0 && i == 0)
        {
            printf("Wasps - Field full!\n");
            free(wasps);
            return 0;
        }
    }

    int key;

    // generate random movements
    while (1)
    {
        // random wasp
        id = rand() % num_wasps;

        // sleep for a random period of time
        usleep(random() % 700000);

        // generate random direction
        wasps[id].direction = (Direction)(rand() % 4);
        wasps[id].has_direction = 1;

        switch (wasps[id].direction)
        {
        case DIRECTION__LEFT:
            printf("Wasp %d Going Left\n", id);
            break;
        case DIRECTION__RIGHT:
            printf("Wasp %d Going Right\n", id);
            break;
        case DIRECTION__DOWN:
            printf("Wasp %d Going Down\n", id);
            break;
        case DIRECTION__UP:
            printf("Wasp %d Going Up\n", id);
            break;
        }

        // send movement message
        msgtype = TYPE__BOT_MOVE;
        zmq_send(requester, &msgtype, sizeof(Type), ZMQ_SNDMORE);

        PACK__REQUEST_MESSAGE(wasps[id], buffer, packed_size);
        SEND__MESSAGE(requester, buffer, packed_size);

        // receive reply
        RECV_UNPACK__REPLY_MESSAGE(requester, reply);
    }

    return 0;
}

/**
 * @brief creates a wasp client and connects to server to add wasps to the game
 *
 * @param argc
 * @param argv server address and request port
 * @return int exit code (0 = success)
 */
int main(int argc, char *argv[])
{
    // argument check
    if (argc != 3)
    {
        printf("Usage: %s <server_address> <req/rep_port>\n", argv[0]);
        return 1;
    }

    // server connection
    char *server_endpoint = (char *)malloc((strlen(argv[1]) + strlen(argv[2]) + 8) * sizeof(char));
    sprintf(server_endpoint, "tcp://%s:%s", argv[1], argv[2]);

    // initialize request socket
    void *context = zmq_ctx_new();
    void *requester = zmq_socket(context, ZMQ_REQ);
    assert(zmq_connect(requester, server_endpoint) == 0);

    // create threads
    pthread_t wasp_thread;
    pthread_create(&wasp_thread, NULL, handle_wasp, (void *)requester);

    // disconnect if enter is pressed
    char key;
    while (1)
    {
        key = getchar();

        if (key == '\n')
        {
            printf("Wasps - Quitting...\n");
            msgtype = TYPE__BOT_DISCONNECT;
            for (int i = 0; i < num_wasps; i++)
            {
                // send disconnect message
                zmq_send(requester, &msgtype, sizeof(Type), ZMQ_SNDMORE);
                PACK__REQUEST_MESSAGE(wasps[i], buffer, packed_size);
                SEND__MESSAGE(requester, buffer, packed_size);

                // receive reply
                RECV_UNPACK__REPLY_MESSAGE(requester, reply);
            }
            break;
        }
    }

    // cleanup and exit
    zmq_close(requester);
    zmq_ctx_destroy(context);
    free(server_endpoint);
    free(wasps);
    free(reply);

    return 0;
}
