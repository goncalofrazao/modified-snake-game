#ifndef __SND_RCV_PROTO_H__
#define __SND_RCV_PROTO_H__

#include "lar-defs.pb-c.h"
#include <zmq.h>

#define PACK__REQUEST_MESSAGE(message, buffer, packed_size)       \
    do                                                            \
    {                                                             \
        packed_size = request_message__get_packed_size(&message); \
        buffer = malloc(packed_size);                             \
        request_message__pack(&message, buffer);                  \
    } while (0);

#define PACK__REPLY_MESSAGE(message, buffer, packed_size)       \
    do                                                          \
    {                                                           \
        packed_size = reply_message__get_packed_size(&message); \
        buffer = malloc(packed_size);                           \
        reply_message__pack(&message, buffer);                  \
    } while (0);

#define PACK__DISPLAY_UPDATE_MESSAGE(message, buffer, packed_size)       \
    do                                                                   \
    {                                                                    \
        packed_size = display_update_message__get_packed_size(&message); \
        buffer = malloc(packed_size);                                    \
        display_update_message__pack(&message, buffer);                  \
    } while (0);

#define SEND__MESSAGE(socket, buffer, packed_size)       \
    do                                                   \
    {                                                    \
        zmq_msg_t msg;                                   \
        zmq_msg_init_size(&msg, packed_size);            \
        memcpy(zmq_msg_data(&msg), buffer, packed_size); \
        zmq_sendmsg(socket, &msg, 0);                    \
        zmq_msg_close(&msg);                             \
        free(buffer);                                    \
    } while (0);

#define RECV_UNPACK__REQUEST_MESSAGE(socket, message)                                    \
    do                                                                                   \
    {                                                                                    \
        zmq_msg_t msg;                                                                   \
        zmq_msg_init(&msg);                                                              \
        zmq_recvmsg(socket, &msg, 0);                                                    \
        message = request_message__unpack(NULL, zmq_msg_size(&msg), zmq_msg_data(&msg)); \
        zmq_msg_close(&msg);                                                             \
    } while (0);

#define RECV_UNPACK__REPLY_MESSAGE(socket, message)                                    \
    do                                                                                 \
    {                                                                                  \
        zmq_msg_t msg;                                                                 \
        zmq_msg_init(&msg);                                                            \
        zmq_recvmsg(socket, &msg, 0);                                                  \
        message = reply_message__unpack(NULL, zmq_msg_size(&msg), zmq_msg_data(&msg)); \
        zmq_msg_close(&msg);                                                           \
    } while (0);

#define RECV_UNPACK__DISPLAY_UPDATE_MESSAGE(socket, message)                                    \
    do                                                                                          \
    {                                                                                           \
        zmq_msg_t msg;                                                                          \
        zmq_msg_init(&msg);                                                                     \
        zmq_recvmsg(socket, &msg, 0);                                                           \
        message = display_update_message__unpack(NULL, zmq_msg_size(&msg), zmq_msg_data(&msg)); \
        zmq_msg_close(&msg);                                                                    \
    } while (0);

#define INVALID_MSG(socket, send_msg)                       \
    do                                                      \
    {                                                       \
        send_msg.success = 0;                               \
        send_msg.has_score = 0;                             \
        send_msg.has_password = 0;                          \
        send_msg.id = NULL;                                 \
        size_t packed_size;                                 \
        void *buffer;                                       \
        PACK__REPLY_MESSAGE(send_msg, buffer, packed_size); \
        SEND__MESSAGE(socket, buffer, packed_size);         \
    } while (0);

#endif