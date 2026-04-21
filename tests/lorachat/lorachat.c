#include "lorachat.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Node, salon, and message queue storage
static node_t known_nodes[MAX_NODES];
static salon_t subscribed_salons[MAX_SALONS];
static message_queue_t message_queue;

// Node and salon counts
static uint8_t node_count = 0;
static uint8_t salon_count = 0;

// Sender id for this device (default 1)
static uint8_t lorachat_sender_id = 1;
// Selected salon for communication (0 = broadcast = * by default)
static uint8_t lorachat_selected_salon = 0;

// Global next message number for this sender
uint16_t lorachat_next_msg_id = 1;

void lorachat_init(void) {
    printf("Initiating LoRa₍ᐢ֎ﻌ֍ᐢ₎ʃ\n");
    printf("[DEBUG] Initializing nodes, salons, and message queue\n");
    memset(known_nodes, 0, sizeof(known_nodes));
    memset(subscribed_salons, 0, sizeof(subscribed_salons));
    memset(&message_queue, 0, sizeof(message_queue));

    // Set default sender id and selected salon
    lorachat_sender_id = 1;
    subscribed_salons[0].salon_id = 0;
    lorachat_selected_salon = 0;
    lorachat_next_msg_id = 1;
}

void lorachat_select_salon(uint8_t salon_id) {
    lorachat_selected_salon = salon_id;
    printf("[DEBUG] Selected salon %u\n", lorachat_selected_salon);
}

void lorachat_construct_message(char *message, char *result) {
    char sender[MAX_NODE_LEN]; sprintf(sender, "%u", lorachat_sender_id);

    char salon[MAX_SALON_LEN]; sprintf(salon, "%u", lorachat_selected_salon);
    if (strcmp(salon, "0") == 0) { strcpy(salon, "*"); }

    char message_id[MAX_MESSAGE_ID_LEN]; sprintf(message_id, "%u", lorachat_next_msg_id);

    snprintf(result, MAX_MESSAGE_LEN, "%s@%s:%s:%s", sender, salon, message_id, message);
    printf("[DEBUG] Built message '%s'\n", result);
}

void lorachat_add_node(uint8_t node_id, uint8_t msg_num) {
    printf("[DEBUG] Adding/updating node %u with msg_num %u\n", node_id, msg_num);
    for (uint8_t i = 0; i < node_count; i++) {
        if (known_nodes[i].node_id == node_id) {
            known_nodes[i].last_msg_num = msg_num;
            printf("[DEBUG] Updated node %u last_msg_num to %u\n", node_id, msg_num);
            return;
        }
    }
    if (node_count < MAX_NODES) {
        known_nodes[node_count].node_id = node_id;
        known_nodes[node_count].last_msg_num = msg_num;
        node_count++;
        printf("[DEBUG] Added new node %u\n", node_id);
    }
}

void lorachat_handle_received_message(char* message) {
    printf("[DEBUG] Received message '%s'\n", message);

    uint8_t message_sender, salon_id, message_id;
    char content[MAX_MESSAGE_CONTENT_LEN];
    char salon_string[MAX_SALON_LEN];

    char message_copy[MAX_MESSAGE_LEN];
    strncpy(message_copy, message, sizeof(message_copy) - 1);
    message_copy[sizeof(message_copy) - 1] = '\0';

    char *token = strtok(message_copy, "@:");
    if (!token) {
        printf("[DEBUG] Failed to parse message: '%s'\n", message);
        return;
    }
    message_sender = (uint8_t)atoi(token);

    token = strtok(NULL, "@:");
    if (!token) {
        printf("[DEBUG] Failed to parse message: '%s'\n", message);
        return;
    }
    strncpy(salon_string, token, MAX_SALON_LEN - 1);
    salon_string[MAX_SALON_LEN - 1] = '\0';

    token = strtok(NULL, "@:");
    if (!token) {
        printf("[DEBUG] Failed to parse message: '%s'\n", message);
        return;
    }
    message_id = (uint8_t)atoi(token);

    token = strtok(NULL, "@:");
    if (!token) {
        printf("[DEBUG] Failed to parse message: '%s'\n", message);
        return;
    }
    strncpy(content, token, MAX_MESSAGE_CONTENT_LEN - 1);
    content[MAX_MESSAGE_CONTENT_LEN - 1] = '\0';

    // Handle salon_id
    if (strcmp(salon_string, "*") == 0) {
        salon_id = 0;
    } else {
        salon_id = (uint8_t)atoi(salon_string);
    }

    printf("[DEBUG] Message sender=%u salon=%u message_id=%u content='%s'\n",
           message_sender, salon_id, message_id, content);

    if (salon_id == 0) {
        printf("[DEBUG] Message for general salon %u\n", salon_id);
        lorachat_enqueue_message(message_sender, salon_id, message_id, content);
        return;
    } else {
        for (uint8_t i = 0; i < salon_count; i++) {
            if (subscribed_salons[i].salon_id == salon_id) {
                printf("[DEBUG] Message for subscribed salon %u\n", salon_id);
                lorachat_enqueue_message(message_sender, salon_id, message_id, content);
                return;
            }
        }
    }
}

void lorachat_add_salon(uint8_t salon_id) {
    printf("[DEBUG] Adding salon %u\n", salon_id);
    for (uint8_t i = 0; i < salon_count; i++) {
        if (subscribed_salons[i].salon_id == salon_id) {
            printf("[DEBUG] Salon %u already subscribed\n", salon_id);
            return;
        }
    }
    if (salon_count < MAX_SALONS) {
        subscribed_salons[salon_count].salon_id = salon_id;
        salon_count++;
        printf("[DEBUG] Salon %u added\n", salon_id);
    }
}

void lorachat_remove_salon(uint8_t salon_id) {
    printf("[DEBUG] lorachat_remove_salon: Removing salon %u\n", salon_id);
    for (uint8_t i = 0; i < salon_count; i++) {
        if (subscribed_salons[i].salon_id == salon_id) {
            for (uint8_t j = i; j + 1 < salon_count; j++) {
                subscribed_salons[j] = subscribed_salons[j + 1];
            }
            memset(&subscribed_salons[salon_count - 1], 0, sizeof(salon_t));
            salon_count--;
            printf("[DEBUG] Salon %u removed\n", salon_id);
            return;
        }
    }
}

void lorachat_enqueue_message(uint8_t sender, uint8_t dest, uint8_t msg_num, const char *content) {
    printf("[DEBUG] sender=%u dest=%u msg_num=%u content='%s'\n", sender, dest, msg_num, content);
    if (message_queue.count >= MAX_MESSAGES) {
        // File full, overwrite oldest message
        printf("[DEBUG] Queue full, overwriting oldest message\n");
        message_queue.head = (message_queue.head + 1) % MAX_MESSAGES;
        message_queue.count--;
    }
    message_queue.messages[message_queue.tail].sender = sender;
    message_queue.messages[message_queue.tail].dest = dest;
    message_queue.messages[message_queue.tail].msg_num = msg_num;
    strncpy(message_queue.messages[message_queue.tail].content, content, MAX_MESSAGE_CONTENT_LEN - 1);
    message_queue.messages[message_queue.tail].content[MAX_MESSAGE_CONTENT_LEN - 1] = '\0';
    message_queue.tail = (message_queue.tail + 1) % MAX_MESSAGES;
    message_queue.count++;
    lorachat_add_node(sender, msg_num);
    printf("[DEBUG] Message enqueued at %u (count=%u)\n", message_queue.tail, message_queue.count);
}

void lorachat_print_nodes(void) {
    printf("[DEBUG] Printing all known nodes\n");
    printf("Known nodes (%u):\n", node_count);
    for (uint8_t i = 0; i < node_count; i++) {
        printf("Node [%u] (last message id: %u)\n", known_nodes[i].node_id, known_nodes[i].last_msg_num);
    }
}

void lorachat_print_messages(void) {
    printf("[DEBUG] Printing all messages\n");
    printf("Messages received (%u):\n", message_queue.count);
    uint8_t index = message_queue.head;
    for (uint8_t i = 0; i < message_queue.count; i++) {
        message_t *msg = &message_queue.messages[index];
        printf("  %u->%u #%u: %s\n", msg->sender, msg->dest, msg->msg_num, msg->content);
        index = (index + 1) % MAX_MESSAGES;
    }
}
