#ifndef LORACHAT_H
#define LORACHAT_H

#include <stdint.h>
#include <stdbool.h>

#define MAX_NODES 32
#define MAX_SALONS 8
#define MAX_MESSAGES 32
#define MAX_MESSAGE_LEN 64
#define MAX_SENSORS 16

typedef struct {
    uint8_t node_id;
    uint8_t last_msg_num;
} node_t;

typedef struct {
    uint8_t salon_id;
    bool subscribed;
} salon_t;

typedef struct {
    uint8_t sender;
    uint8_t dest;
    uint8_t msg_num;
    char content[MAX_MESSAGE_LEN];
} message_t;

typedef struct {
    message_t messages[MAX_MESSAGES];
    uint8_t head;
    uint8_t tail;
    uint8_t count;
} message_queue_t;

/* Init lorachat global variables */
void lorachat_init(void);
/* Construct a message to send */
void lorachat_construct_message(char *message, char *result);
/* Handle a received message */
void lorachat_handle_received_message(char* message);
/* Subscribe to a salon */
void lorachat_add_salon(uint8_t salon_id);
/* Unsubscribe from a salon */
void lorachat_remove_salon(uint8_t salon_id);
/* List known nodes and their last message numbers */
void lorachat_print_nodes(void);
/* List messages in the queue */
void lorachat_print_messages(void);

#endif
