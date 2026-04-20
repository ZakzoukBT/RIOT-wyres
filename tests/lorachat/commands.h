#ifndef COMMANDS_H
#define COMMANDS_H

#include "lorachat.h"
#include "shell.h"

int lora_setup_cmd(int argc, char **argv);
int random_cmd(int argc, char **argv);
int register_cmd(int argc, char **argv);
int send_cmd(int argc, char **argv);
int send_hex_cmd(int argc, char **argv);
int echo_cmd(int argc, char **argv);
int rxhex_cmd(int argc, char **argv);
int listen_cmd(int argc, char **argv);
int syncword_cmd(int argc, char **argv);
int channel_cmd(int argc, char **argv);
int rx_timeout_cmd(int argc, char **argv);
int reset_cmd(int argc, char **argv);
int crc_cmd(int argc, char **argv);
int implicit_cmd(int argc, char **argv);
int payload_cmd(int argc, char **argv);
int init_sx1272_cmd(int argc, char **argv);
int lorachat_send_cmd(int argc, char **argv);
int nodes_cmd(int argc, char **argv);
int salons_cmd(int argc, char **argv);
int messages_cmd(int argc, char **argv);

#endif
