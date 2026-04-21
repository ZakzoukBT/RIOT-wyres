#ifndef LORACHAT_H
#define LORACHAT_H

#include <stdint.h>
#include <stdbool.h>

#define MAX_NODES 32
#define MAX_NODE_LEN 8
#define MAX_SALONS 8
#define MAX_SALON_LEN 8
#define MAX_MESSAGES 32
#define MAX_MESSAGE_CONTENT_LEN 64
#define MAX_MESSAGE_ID_LEN 8
#define MAX_MESSAGE_LEN (MAX_NODE_LEN+1+MAX_SALON_LEN+1+MAX_MESSAGE_ID_LEN+1+MAX_MESSAGE_CONTENT_LEN)
#define MAX_SENSORS 16

typedef struct {
    uint8_t node_id;
    uint8_t last_msg_num;
} node_t;

typedef struct {
    uint8_t salon_id;
} salon_t;

typedef struct {
    uint8_t sender;
    uint8_t dest;
    uint8_t msg_num;
    char content[MAX_MESSAGE_CONTENT_LEN];
} message_t;

typedef struct {
    message_t messages[MAX_MESSAGES];
    uint8_t head;
    uint8_t tail;
    uint8_t count;
} message_queue_t;

/* Global next message id for this sender */
extern uint16_t lorachat_next_msg_id;

/* Init lorachat global variables */
void lorachat_init(void);
/* Construct a message to send */
void lorachat_construct_message(char *message, char *result);
/* Handle a received message */
void lorachat_handle_received_message(char* message);
/* Set the salon to listen to */
void lorachat_select_salon(uint8_t salon_id);
/* Subscribe to a salon */
void lorachat_add_salon(uint8_t salon_id);
/* Unsubscribe from a salon */
void lorachat_remove_salon(uint8_t salon_id);
/* List known nodes and their last message numbers */
void lorachat_print_nodes(void);
/* List messages in the queue */
void lorachat_print_messages(void);
/* Enqueue a received message */
void lorachat_enqueue_message(uint8_t sender, uint8_t dest, uint8_t msg_num, const char *content);

#endif
