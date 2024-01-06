#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <assert.h>

#include "../snd_rcv_proto.h"

#define MAX_ROACHES 10

int main(int argc, char *argv[])
{
    int id;
    int num_roaches;

    Type msgtype = TYPE__ROACH_CONNECT;
    RequestMessage *roaches;
    ReplyMessage *reply;
    void *buffer;
    size_t packed_size;

    // argument check
    if (argc != 3)
    {
        printf("Usage: %s <server_address> <req/rep_port>\n", argv[0]);
        return 1;
    }

    // server connection
    char *server_endpoint = (char *)malloc((strlen(argv[1]) + strlen(argv[2]) + 8) * sizeof(char));
    sprintf(server_endpoint, "tcp://%s:%s", argv[1], argv[2]);
    void *context = zmq_ctx_new();
    void *requester = zmq_socket(context, ZMQ_REQ);
    assert(zmq_connect(requester, server_endpoint) == 0);

    // randomly choose number of roaches
    srand(time(NULL));
    num_roaches = rand() % MAX_ROACHES + 1;
    roaches = (RequestMessage *)malloc(num_roaches * sizeof(RequestMessage));

    // connect possible roaches to server
    for (int i = 0; i < num_roaches; i++)
    {
        // connect message
        zmq_send(requester, &msgtype, sizeof(Type), ZMQ_SNDMORE);
        request_message__init(&roaches[i]);
        roaches[i].id = (char *)malloc(2 * sizeof(char));
        roaches[i].id[0] = rand() % 5 + '1';
        roaches[i].id[1] = '\0';
        roaches[i].has_direction = 0;
        roaches[i].has_password = 0;
        PACK__REQUEST_MESSAGE(roaches[i], buffer, packed_size);
        SEND__MESSAGE(requester, buffer, packed_size);

        // receive password
        RECV_UNPACK__REPLY_MESSAGE(requester, reply);

        roaches[i].password = reply->password;
        roaches[i].has_password = 1;

        // full field
        if (roaches[i].password == 0 && i != 0)
        {
            printf("Field full of roaches!\n");
            num_roaches = i;
            roaches = (RequestMessage *)realloc(roaches, num_roaches * sizeof(RequestMessage));
            break;
        }
        else if (roaches[i].password == 0 && i == 0)
        {
            printf("Field full of roaches!\n");
            free(roaches);
            return 0;
        }
    }

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
        msgtype = TYPE__ROACH_MOVE;
        zmq_send(requester, &msgtype, sizeof(Type), ZMQ_SNDMORE);

        PACK__REQUEST_MESSAGE(roaches[id], buffer, packed_size);
        SEND__MESSAGE(requester, buffer, packed_size);

        // receive reply
        RECV_UNPACK__REPLY_MESSAGE(requester, reply);
    }

    // cleanup and exit
    zmq_close(requester);
    zmq_ctx_destroy(context);
    free(server_endpoint);
    free(roaches);

    return 0;
}
