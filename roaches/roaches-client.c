#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <assert.h>
#include <pthread.h>

#include "../snd_rcv_proto.h"

#define MAX_ROACHES 10

static Type msgtype = TYPE__BOT_CONNECT;
static RequestMessage *roaches;
static ReplyMessage *reply;
static void *buffer;
static size_t packed_size;
static int id;
static int num_roaches;

/**
 * @brief thread function to handle roaches movements and connections
 *
 * @param arg requester socket
 * @return void*
 */
void *handle_roach(void *arg)
{
    void *requester = (void *)arg;

    // randomly choose number of roaches
    srand(time(NULL));
    num_roaches = rand() % MAX_ROACHES + 1;
    roaches = (RequestMessage *)malloc(num_roaches * sizeof(RequestMessage));

    // connect possible roaches to server
    for (int i = 0; i < num_roaches; i++)
    {
        // send connect type message and setup roach
        zmq_send(requester, &msgtype, sizeof(Type), ZMQ_SNDMORE);
        request_message__init(&roaches[i]);
        roaches[i].id = (char *)malloc(2 * sizeof(char));
        roaches[i].id[0] = rand() % 5 + '1';
        roaches[i].id[1] = '\0';
        roaches[i].has_direction = 0;
        roaches[i].has_password = 0;

        // pack and send the connect message
        PACK__REQUEST_MESSAGE(roaches[i], buffer, packed_size);
        SEND__MESSAGE(requester, buffer, packed_size);

        // receive password
        RECV_UNPACK__REPLY_MESSAGE(requester, reply);

        // save password
        roaches[i].password = reply->password;
        roaches[i].has_password = 1;

        // full field
        if (roaches[i].password == 0 && i != 0)
        {
            printf("Field full!\n");
            num_roaches = i;
            // resize roaches array
            roaches = (RequestMessage *)realloc(roaches, num_roaches * sizeof(RequestMessage));
            break;
        }
        else if (roaches[i].password == 0 && i == 0)
        {
            printf("Field full!\n");
            exit(0);
        }
    }

    int key;

    // generate random movements
    while (1)
    {
        // random roach
        id = rand() % num_roaches;

        // sleep for a random period of time
        usleep(random() % 700000);

        // generate random direction
        roaches[id].direction = (Direction)(rand() % 4);
        roaches[id].has_direction = 1;

        switch (roaches[id].direction)
        {
        case DIRECTION__LEFT:
            printf("Roach %d Going Left\n", id);
            break;
        case DIRECTION__RIGHT:
            printf("Roach %d Going Right\n", id);
            break;
        case DIRECTION__DOWN:
            printf("Roach %d Going Down\n", id);
            break;
        case DIRECTION__UP:
            printf("Roach %d Going Up\n", id);
            break;
        }

        // send movement message
        msgtype = TYPE__BOT_MOVE;
        zmq_send(requester, &msgtype, sizeof(Type), ZMQ_SNDMORE);

        PACK__REQUEST_MESSAGE(roaches[id], buffer, packed_size);
        SEND__MESSAGE(requester, buffer, packed_size);

        // receive reply
        RECV_UNPACK__REPLY_MESSAGE(requester, reply);
    }

    return 0;
}

/**
 * @brief creates a roach client and connects to server to add roaches to the game
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
    pthread_t roach_thread;
    pthread_create(&roach_thread, NULL, handle_roach, (void *)requester);

    // disconnect if enter is pressed
    char key;
    while (1)
    {
        key = getchar();

        if (key == '\n')
        {
            printf("Disconnecting...\n");
            msgtype = TYPE__BOT_DISCONNECT;
            for (int i = 0; i < num_roaches; i++)
            {
                // send disconnect message
                zmq_send(requester, &msgtype, sizeof(Type), ZMQ_SNDMORE);
                PACK__REQUEST_MESSAGE(roaches[i], buffer, packed_size);
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
    free(roaches);
    free(reply);

    return 0;
}
