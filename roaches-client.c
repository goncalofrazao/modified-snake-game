#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

#include "zhelpers.h"
#include "lar-defs.h"

#define MAX_ROACHES 10

int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("Usage: %s <server_address> <server_port>\n", argv[0]);
        return 1;
    }

    // Get Server Address and Port
    char *server_address = argv[1];
    int server_port = atoi(argv[2]);

    // Create socket
    void *context = zmq_ctx_new();
    void *requester = zmq_socket(context, ZMQ_REQ);

    // Connect to reply socket
    char server_endpoint[256];
    sprintf(server_endpoint, "tcp://%s:%d", server_address, server_port);
    int rc = zmq_connect(requester, server_endpoint);
    assert(rc == 0);

    // Seed random number generator
    srand(time(NULL));
    // int num_roaches = rand() % MAX_ROACHES + 1;
    int num_roaches = 350; 
    msg_t roaches[MAX_ROACHES];
    reply_t reply;

    printf("Number of roaches: %d\n", num_roaches);
        
    //Connect every roach to server and get password
    for (int i = 0; i < num_roaches; i++) {
        printf("Roach %d Connecting\n", i);
        roaches[i].type = ROACH_CONNECT;
        roaches[i].id = rand() % 5 + '1';
        zmq_send(requester, &roaches[i], sizeof(roaches[i]), 0);
        zmq_recv(requester, &reply, sizeof(reply), 0);
        roaches[i].password = reply.password;
        roaches[i].type = ROACH_MOVE;
        printf("Password: %d\n", roaches[i].password);
        printf("Roach %d.type: %d\n", i, roaches[i].type);
        //If field is full of roaches, stop creating roaches
        if (roaches[i].password == -1 && i != 0){
            printf("Field full of roaches!\n");
            num_roaches = i;
            break;
        }
        else if (roaches[i].password == -1 && i == 0){
            printf("Field full of roaches!\n");
            return 0;
        }
    }

    //Create direction variable
    direction_t dir;
    while (1) {
        //Choose a random roach to move
        int i = rand() % num_roaches;

        // Sleep for a random period of time
        usleep(random() % 700000);

        // Generate random movement
        dir = (direction_t) (rand() % 4);
        switch (dir){
        case LEFT:
            printf("Roach %d Going Left\n", i);
            break;
        case RIGHT:
            printf("Roach %d Going Right\n", i);
            break;
        case DOWN:
            printf("Roach %d Going Down\n", i);
            break;
        case UP:
            printf("Roach %d Going Up\n", i);
            break;
        }

        // Send movement to server
        roaches[i].direction = dir;
        zmq_send(requester, &roaches[i], sizeof(msg_t), 0);
        zmq_recv(requester, &reply, sizeof(reply_t), 0);
    }

    zmq_close(requester);
    zmq_ctx_destroy(context);

    return 0;
}
