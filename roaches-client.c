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

    if (argc != 3) {
        printf("Usage: %s <server_address> <req/rep_port>\n", argv[0]);
        return 1;
    }

    char *server_endpoint = (char*) malloc((strlen(argv[1]) + strlen(argv[2]) + 8) * sizeof(char));
    sprintf(server_endpoint, "tcp://%s:%s", argv[1], argv[2]);

    // Create socket
    void *context = zmq_ctx_new();
    void *requester = zmq_socket(context, ZMQ_REQ);
    assert(zmq_connect(requester, server_endpoint) == 0);
    
    srand(time(NULL));
    // num_roaches = rand() % MAX_ROACHES + 1;
    num_roaches = 3;
    roaches = (msg_t*) malloc(num_roaches * sizeof(msg_t));
    //Connect every roach to server and get password
    for (int i = 0; i < num_roaches; i++) {
        roaches[i].type = ROACH_CONNECT;
        roaches[i].id = rand() % 5 + '1';
        assert(zmq_send(requester, &roaches[i], sizeof(roaches[i]), 0) == sizeof(roaches[i]));

        assert(zmq_recv(requester, &reply, sizeof(reply), 0) == sizeof(reply));
        roaches[i].password = reply.password;
        roaches[i].type = ROACH_MOVE;

        //If field is full of roaches, stop creating roaches
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

    //Create direction variable
    while (1) {
        //Choose a random roach to move
        id = rand() % num_roaches;

        // Sleep for a random period of time
        usleep(random() % 700000);

        // Generate random movement
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

        // Send movement to server
        roaches[id].direction = dir;
        assert(zmq_send(requester, &roaches[id], sizeof(msg_t), 0) == sizeof(msg_t));
        assert(zmq_recv(requester, &reply, sizeof(reply_t), 0) == sizeof(reply_t));
    }

    zmq_close(requester);
    zmq_ctx_destroy(context);
    free(server_endpoint);
    free(roaches);

    return 0;
}
