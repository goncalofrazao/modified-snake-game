#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

#include "zhelpers.h"
#include "lar-defs.h"

#define MAX_ROACHES 10

int main(int argc, char *argv[]) {
    reply_t reply;
    direction_t dir;
    int id;
    int num_roaches;
    msg_t *roaches;

    // argument check
    if (argc != 3) {
        printf("Usage: %s <server_address> <req/rep_port>\n", argv[0]);
        return 1;
    }

    // server connection
    char *server_endpoint = (char*) malloc((strlen(argv[1]) + strlen(argv[2]) + 8) * sizeof(char));
    sprintf(server_endpoint, "tcp://%s:%s", argv[1], argv[2]);
    void *context = zmq_ctx_new();
    void *requester = zmq_socket(context, ZMQ_REQ);
    assert(zmq_connect(requester, server_endpoint) == 0);
    
    // randomly choose number of roaches
    srand(time(NULL));
    num_roaches = rand() % MAX_ROACHES + 1;
    roaches = (msg_t*) malloc(num_roaches * sizeof(msg_t));

    // connect possible roaches to server
    for (int i = 0; i < num_roaches; i++) {
        // connect message
        roaches[i].type = ROACH_CONNECT;
        roaches[i].id = rand() % 5 + '1';
        assert(zmq_send(requester, &roaches[i], sizeof(roaches[i]), 0) == sizeof(roaches[i]));

        // receive password
        assert(zmq_recv(requester, &reply, sizeof(reply), 0) == sizeof(reply));
        roaches[i].password = reply.password;
        roaches[i].type = ROACH_MOVE;

        // full field
        if (roaches[i].password == -1 && i != 0){
            printf("Field full of roaches!\n");
            num_roaches = i;
            roaches = (msg_t*) realloc(roaches, num_roaches * sizeof(msg_t));
            break;
        }
        else if (roaches[i].password == -1 && i == 0){
            printf("Field full of roaches!\n");
            free(roaches);
            return 0;
        }
    }

    // generate random movements
    while (1) {
        // random roach
        id = rand() % num_roaches;

        // sleep for a random period of time
        usleep(random() % 700000);

        // generate random direction
        dir = (direction_t) (rand() % 4);
        switch (dir){
        case LEFT:
            printf("Roach %d Going Left\n", id);
            break;
        case RIGHT:
            printf("Roach %d Going Right\n", id);
            break;
        case DOWN:
            printf("Roach %d Going Down\n", id);
            break;
        case UP:
            printf("Roach %d Going Up\n", id);
            break;
        }

        // send movement message
        roaches[id].direction = dir;
        assert(zmq_send(requester, &roaches[id], sizeof(msg_t), 0) == sizeof(msg_t));
        assert(zmq_recv(requester, &reply, sizeof(reply_t), 0) == sizeof(reply_t));
    }

    // cleanup and exit
    zmq_close(requester);
    zmq_ctx_destroy(context);
    free(server_endpoint);
    free(roaches);

    return 0;
}
