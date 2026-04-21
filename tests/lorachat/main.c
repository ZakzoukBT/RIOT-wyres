/*
 * Copyright (C) 2016 Unwired Devices <info@unwds.com>
 *               2017 Inria Chile
 *               2017 Inria
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @ingroup     tests
 * @{
 * @file
 * @brief       Test application for SX127X modem driver
 *
 * @author      Eugene P. <ep@unwds.com>
 * @author      José Ignacio Alamos <jose.alamos@inria.cl>
 * @author      Alexandre Abadie <alexandre.abadie@inria.fr>
 * @}
 */

#include "commands.h"
#include <string.h>

// static const shell_command_t shell_commands[] = {
//     {"init", "Initialize SX1272", init_sx1272_cmd},
//     {"setup", "Initialize LoRa modulation settings", lora_setup_cmd},
//     {"implicit", "Enable implicit header", implicit_cmd},
//     {"crc", "Enable CRC", crc_cmd},
//     {"payload", "Set payload length (implicit header)", payload_cmd},
//     {"random", "Get random number from sx127x", random_cmd},
//     {"syncword", "Get/Set the syncword", syncword_cmd},
//     {"rx_timeout", "Set the RX timeout", rx_timeout_cmd},
//     {"channel", "Get/Set channel frequency (in Hz)", channel_cmd},
//     {"register", "Get/Set value(s) of registers of sx127x", register_cmd},
//     {"send", "Send raw payload string", send_cmd},
//     {"send_hex", "Send payload in hexadecimal", send_hex_cmd},
//     {"rxhex", "Enable/disable RX hexadecimal display", rxhex_cmd},
//     {"listen", "Start raw payload listener", listen_cmd},
//     {"echo", "Enable/disable echo mode (re-send received payload)", echo_cmd},
//     {"reset", "Reset the sx127x device", reset_cmd},
//     {NULL, NULL, NULL}};

static const shell_command_t lorachat_shell_commands[] = {
    {"listen", "Set listen mode", listen_cmd},
    {"salon", "Select the salon to send messages in", lorachat_select_salon_cmd},
    {"send", "Send a message in the selected salon", lorachat_send_cmd},
    {"salons", "Subscribe/Unsubscribe from a salon", lorachat_salons_cmd},
    {"nodes", "List available nodes", lorachat_nodes_cmd},
    {"messages", "List messages in the selected salon", lorachat_messages_cmd},
    {NULL, NULL, NULL}
};

int main(void)
{
    init_sx1272_cmd(0, NULL);
    char line_buf[SHELL_DEFAULT_BUFSIZE];
    shell_run(lorachat_shell_commands, line_buf, SHELL_DEFAULT_BUFSIZE);

    return 0;
}
