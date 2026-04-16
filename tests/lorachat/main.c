#include <errno.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "thread.h"
#include "shell.h"

#include "net/netdev.h"
#include "net/netdev/lora.h"
#include "net/lora.h"

#include "sx127x_internal.h"
#include "sx127x_params.h"
#include "sx127x_netdev.h"

#define LORACHAT_MSG_QUEUE (16U)
#ifndef SX127X_STACKSIZE
#define SX127X_STACKSIZE (THREAD_STACKSIZE_DEFAULT)
#endif

#define MSG_TYPE_ISR (0x3456)
#define CHAT_RX_BUF_SIZE (255U)
#define CHAT_TX_BUF_SIZE (255U)
#define CHAT_DEFAULT_ID (1U)

static char stack[SX127X_STACKSIZE];
static kernel_pid_t _recv_pid;

static sx127x_t sx127x;
static uint16_t chat_local_id = CHAT_DEFAULT_ID;
static const char *CHAT_BROADCAST_ADDR = "*";

static int lora_setup_cmd(int argc, char **argv)
{
    if (argc < 4)
    {
        puts("usage: setup <bandwidth (125, 250, 500)> <spreading factor (7..12)> <code rate (5..8)>");
        return -1;
    }

    int bw = atoi(argv[1]);
    uint8_t lora_bw;

    switch (bw)
    {
    case 125:
        lora_bw = LORA_BW_125_KHZ;
        break;
    case 250:
        lora_bw = LORA_BW_250_KHZ;
        break;
    case 500:
        lora_bw = LORA_BW_500_KHZ;
        break;
    default:
        puts("setup: invalid bandwidth value, use 125, 250 or 500");
        return -1;
    }

    uint8_t lora_sf = (uint8_t)atoi(argv[2]);
    if ((lora_sf < 7U) || (lora_sf > 12U))
    {
        puts("setup: invalid spreading factor value, use 7..12");
        return -1;
    }

    int cr = atoi(argv[3]);
    if ((cr < 5) || (cr > 8))
    {
        puts("setup: invalid coding rate value, use 5..8");
        return -1;
    }
    uint8_t lora_cr = (uint8_t)(cr - 4);

    netdev_t *netdev = &sx127x.netdev;

    netdev->driver->set(netdev, NETOPT_BANDWIDTH, &lora_bw, sizeof(lora_bw));
    netdev->driver->set(netdev, NETOPT_SPREADING_FACTOR, &lora_sf, sizeof(lora_sf));
    netdev->driver->set(netdev, NETOPT_CODING_RATE, &lora_cr, sizeof(lora_cr));

    puts("LoRa settings updated");
    return 0;
}

static int channel_cmd(int argc, char **argv)
{
    if (argc < 2)
    {
        puts("usage: channel <get|set>");
        return -1;
    }

    netdev_t *netdev = &sx127x.netdev;
    uint32_t chan;

    if (strcmp(argv[1], "get") == 0)
    {
        netdev->driver->get(netdev, NETOPT_CHANNEL_FREQUENCY, &chan, sizeof(chan));
        printf("Channel: %" PRIu32 "\n", chan);
        return 0;
    }

    if (strcmp(argv[1], "set") == 0)
    {
        if (argc < 3)
        {
            puts("usage: channel set <channel_hz>");
            return -1;
        }

        chan = (uint32_t)strtoul(argv[2], NULL, 10);
        netdev->driver->set(netdev, NETOPT_CHANNEL_FREQUENCY, &chan, sizeof(chan));
        printf("Channel set to %" PRIu32 "\n", chan);
        return 0;
    }

    puts("usage: channel <get|set>");
    return -1;
}

static int listen_cmd(int argc, char **argv)
{
    (void)argc;
    (void)argv;

    netdev_t *netdev = &sx127x.netdev;

    const netopt_enable_t single = false;
    const uint32_t timeout = 0;

    netdev->driver->set(netdev, NETOPT_SINGLE_RECEIVE, &single, sizeof(single));
    netdev->driver->set(netdev, NETOPT_RX_TIMEOUT, &timeout, sizeof(timeout));

    netopt_state_t state = NETOPT_STATE_RX;
    netdev->driver->set(netdev, NETOPT_STATE, &state, sizeof(state));

    puts("Listen mode enabled");
    return 0;
}

static int id_cmd(int argc, char **argv)
{
    if (argc < 2)
    {
        puts("usage: id <get|set>");
        return -1;
    }

    if (strcmp(argv[1], "get") == 0)
    {
        printf("local id: %u\n", (unsigned)chat_local_id);
        return 0;
    }

    if (strcmp(argv[1], "set") == 0)
    {
        if (argc < 3)
        {
            puts("usage: id set <1..65535>");
            return -1;
        }

        unsigned long value = strtoul(argv[2], NULL, 10);
        if ((value == 0UL) || (value > 65535UL))
        {
            puts("invalid id: must be between 1 and 65535");
            return -1;
        }

        chat_local_id = (uint16_t)value;
        printf("local id set to %u\n", (unsigned)chat_local_id);
        return 0;
    }

    puts("usage: id <get|set>");
    return -1;
}

static bool _parse_chat_frame(char *frame, uint16_t *src, uint16_t *dst,
                              bool *is_broadcast, char **msg)
{
    char *at = strchr(frame, '@');
    char *sep = strchr(frame, ':');

    if ((at == NULL) || (sep == NULL) || (at > sep))
    {
        return false;
    }

    *at = '\0';
    *sep = '\0';

    char *src_s = frame;
    char *dst_s = at + 1;
    char *msg_s = sep + 1;

    if ((*src_s == '\0') || (*dst_s == '\0') || (*msg_s == '\0'))
    {
        return false;
    }

    char *endptr = NULL;
    unsigned long src_v = strtoul(src_s, &endptr, 10);
    if ((*endptr != '\0') || (src_v == 0UL) || (src_v > 65535UL))
    {
        return false;
    }

    if (strcmp(dst_s, CHAT_BROADCAST_ADDR) == 0)
    {
        *is_broadcast = true;
        *dst = 0U;
    }
    else
    {
        unsigned long dst_v = strtoul(dst_s, &endptr, 10);
        if ((*endptr != '\0') || (dst_v == 0UL) || (dst_v > 65535UL))
        {
            return false;
        }

        *is_broadcast = false;
        *dst = (uint16_t)dst_v;
    }

    *src = (uint16_t)src_v;
    *msg = msg_s;

    return true;
}

static int chat_send_cmd(int argc, char **argv)
{
    if (argc < 3)
    {
        puts("usage: chat_send <dest_id|*> <message>");
        return -1;
    }

    char frame[CHAT_TX_BUF_SIZE];
    int len;
    if (strcmp(argv[1], CHAT_BROADCAST_ADDR) == 0)
    {
        len = snprintf(frame, sizeof(frame), "%u@%s:%s",
                       (unsigned)chat_local_id, CHAT_BROADCAST_ADDR, argv[2]);
    }
    else
    {
        char *endptr = NULL;
        unsigned long dst_v = strtoul(argv[1], &endptr, 10);
        if ((*endptr != '\0') || (dst_v == 0UL) || (dst_v > 65535UL))
        {
            puts("invalid destination id: use * or a value between 1 and 65535");
            return -1;
        }

        len = snprintf(frame, sizeof(frame), "%u@%lu:%s",
                       (unsigned)chat_local_id, dst_v, argv[2]);
    }

    if ((len < 0) || ((size_t)len >= sizeof(frame)))
    {
        puts("message too long for frame buffer");
        return -1;
    }

    iolist_t iolist = {
        .iol_base = frame,
        .iol_len = (size_t)len};

    netdev_t *netdev = &sx127x.netdev;
    if (netdev->driver->send(netdev, &iolist) == -ENOTSUP)
    {
        puts("Cannot send: radio is still transmitting");
        return -1;
    }

    printf("chat frame sent: %s\n", frame);
    return 0;
}

static void _event_cb(netdev_t *dev, netdev_event_t event)
{
    if (event == NETDEV_EVENT_ISR)
    {
        msg_t msg;
        msg.type = MSG_TYPE_ISR;
        msg.content.ptr = dev;

        if (msg_send(&msg, _recv_pid) <= 0)
        {
            puts("gnrc_netdev: possibly lost interrupt");
        }
        return;
    }

    switch (event)
    {
    case NETDEV_EVENT_RX_STARTED:
        puts("RX started");
        break;

    case NETDEV_EVENT_RX_COMPLETE:
    {
        size_t len = dev->driver->recv(dev, NULL, 0, 0);
        netdev_lora_rx_info_t packet_info;
        char rx_buf[CHAT_RX_BUF_SIZE];

        if (len >= sizeof(rx_buf))
        {
            len = sizeof(rx_buf) - 1;
        }

        dev->driver->recv(dev, rx_buf, len, &packet_info);
        rx_buf[len] = '\0';

        printf("RX frame: \"%s\" (%u bytes), RSSI: %i, SNR: %i, TOA: %" PRIu32 "\n",
               rx_buf, (unsigned)len,
               packet_info.rssi, (int)packet_info.snr,
               sx127x_get_time_on_air((const sx127x_t *)dev, len));

        uint16_t src, dst;
        bool is_broadcast = false;
        char *msg;
        if (_parse_chat_frame(rx_buf, &src, &dst, &is_broadcast, &msg))
        {
            if (is_broadcast)
            {
                printf("[chat] %u -> *: %s\n", (unsigned)src, msg);
            }
            else if (dst == chat_local_id)
            {
                printf("[chat] %u -> %u: %s\n", (unsigned)src, (unsigned)dst, msg);
            }
            else
            {
                printf("[chat] frame ignored (dest=%u, local=%u)\n",
                       (unsigned)dst, (unsigned)chat_local_id);
            }
        }
        else
        {
            puts("[chat] invalid frame format, expected src@dst:message or src@*:message");
        }
        break;
    }

    case NETDEV_EVENT_TX_COMPLETE:
        sx127x_set_sleep(&sx127x);
        puts("Transmission completed");
        break;

    case NETDEV_EVENT_TX_TIMEOUT:
        sx127x_set_sleep(&sx127x);
        puts("Transmission timeout");
        break;

    case NETDEV_EVENT_CAD_DONE:
        break;

    default:
        printf("Unexpected netdev event: %d\n", event);
        break;
    }
}

static void *_recv_thread(void *arg)
{
    (void)arg;

    static msg_t _msg_q[LORACHAT_MSG_QUEUE];
    msg_init_queue(_msg_q, LORACHAT_MSG_QUEUE);

    while (1)
    {
        msg_t msg;
        msg_receive(&msg);

        if (msg.type == MSG_TYPE_ISR)
        {
            netdev_t *dev = msg.content.ptr;
            dev->driver->isr(dev);
        }
        else
        {
            puts("Unexpected msg type");
        }
    }

    return NULL;
}

static int init_cmd(int argc, char **argv)
{
    (void)argc;
    (void)argv;

    sx127x.params = sx127x_params[0];

    netdev_t *netdev = &sx127x.netdev;
    netdev->driver = &sx127x_driver;
    netdev->event_callback = _event_cb;

    if (netdev->driver->init(netdev) < 0)
    {
        puts("Failed to initialize SX127x");
        return 1;
    }

    _recv_pid = thread_create(stack, sizeof(stack), THREAD_PRIORITY_MAIN - 1,
                              THREAD_CREATE_STACKTEST, _recv_thread, NULL,
                              "lorachat_recv");

    if (_recv_pid <= KERNEL_PID_UNDEF)
    {
        puts("Creation of receiver thread failed");
        return 1;
    }

    printf("SX127x initialized, local id=%u\n", (unsigned)chat_local_id);
    return 0;
}

static const shell_command_t shell_commands[] = {
    {"init", "Initialize SX127x", init_cmd},
    {"setup", "Set LoRa modulation settings", lora_setup_cmd},
    {"channel", "Get/Set channel frequency in Hz", channel_cmd},
    {"listen", "Start continuous listener", listen_cmd},
    {"id", "Get/Set local chat ID", id_cmd},
    {"chat_send", "Send chat frame to destination ID", chat_send_cmd},
    {NULL, NULL, NULL}};

int main(void)
{
    puts("LoRaChat ready - type 'help' for commands");

    char line_buf[SHELL_DEFAULT_BUFSIZE];
    shell_run(shell_commands, line_buf, SHELL_DEFAULT_BUFSIZE);

    return 0;
}
