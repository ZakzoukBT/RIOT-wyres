#include "lorachat.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>

// Node, salon, and message queue storage
static node_t known_nodes[MAX_NODES];
static salon_t subscribed_salons[MAX_SALONS];
static message_queue_t message_queue;

// Node and salon counts
static uint8_t node_count = 0;
static uint8_t salon_count = 0;

// Global sender id for this device (default 1)
static uint8_t lorachat_sender_id = 1;
// Global selected salon for communication (0 = broadcast = * by default)
static uint8_t lorachat_selected_salon = 0;

void lorachat_init(void) {
    memset(known_nodes, 0, sizeof(known_nodes));
    memset(subscribed_salons, 0, sizeof(subscribed_salons));
    memset(&message_queue, 0, sizeof(message_queue));

    // Set default sender id and selected salon
    lorachat_sender_id = 1;
    lorachat_selected_salon = 0;
}

void lorachat_construct_message(char *message, char *result) {
    char sender[8]; sprintf(sender, "%u", lorachat_sender_id);

    char salon[8]; sprintf(salon, "%u", lorachat_selected_salon);
    if (strcmp(salon, "0") == 0) { strcpy(salon, "*"); }

    sprintf(result, "%s@%s:%s", sender, salon, message);
}

void lorachat_add_node(uint8_t node_id, uint8_t msg_num) {
    for (uint8_t i = 0; i < node_count; i++) {
        if (known_nodes[i].node_id == node_id) {
            known_nodes[i].last_msg_num = msg_num;
            return;
        }
    }
    if (node_count < MAX_NODES) {
        known_nodes[node_count].node_id = node_id;
        known_nodes[node_count].last_msg_num = msg_num;
        node_count++;
    }
}

void lorachat_handle_received_message(char* message){
    uint8_t message_sender, salon, message_id;
    char content[8];

    sscanf(message, "%hhu@%hhu:%hhu:%s", &message_sender, &salon, &message_id, content);

    for (uint8_t i = 0; i < salon_count; i++) {
        if (subscribed_salons[i].salon_id == salon) {
            lorachat_add_node(message_sender, message_id);

        }
    }
}

void lorachat_add_salon(uint8_t salon_id) {
    for (uint8_t i = 0; i < salon_count; i++) {
        if (subscribed_salons[i].salon_id == salon_id) {
            subscribed_salons[i].subscribed = true;
            return;
        }
    }
    if (salon_count < MAX_SALONS) {
        subscribed_salons[salon_count].salon_id = salon_id;
        subscribed_salons[salon_count].subscribed = true;
        salon_count++;
    }
}

void lorachat_remove_salon(uint8_t salon_id) {
    for (uint8_t i = 0; i < salon_count; i++) {
        if (subscribed_salons[i].salon_id == salon_id) {
            subscribed_salons[i].subscribed = false;
            return;
        }
    }
}

void lorachat_enqueue_message(uint8_t sender, uint8_t dest, uint8_t msg_num, const char *content) {
    if (message_queue.count >= MAX_MESSAGES) {
        // File pleine, on écrase le plus ancien
        message_queue.head = (message_queue.head + 1) % MAX_MESSAGES;
        message_queue.count--;
    }
    message_queue.messages[message_queue.tail].sender = sender;
    message_queue.messages[message_queue.tail].dest = dest;
    message_queue.messages[message_queue.tail].msg_num = msg_num;
    strncpy(message_queue.messages[message_queue.tail].content, content, MAX_MESSAGE_LEN - 1);
    message_queue.tail = (message_queue.tail + 1) % MAX_MESSAGES;
    message_queue.count++;
    lorachat_add_node(sender, msg_num);
}

void lorachat_print_nodes(void) {
    printf("Known nodes (%u):\n", node_count);
    for (uint8_t i = 0; i < node_count; i++) {
        printf("Node [%u] (last message: %u)\n", known_nodes[i].node_id, known_nodes[i].last_msg_num);
    }
}

void lorachat_print_messages(void) {
    printf("Messages received (%u):\n", message_queue.count);
    uint8_t index = message_queue.head;
    for (uint8_t i = 0; i < message_queue.count; i++) {
        message_t *msg = &message_queue.messages[index];
        printf("  %u->%u #%u: %s\n", msg->sender, msg->dest, msg->msg_num, msg->content);
        index = (index + 1) % MAX_MESSAGES;
    }
}
