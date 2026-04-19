#include "hex.h"
#include <stdio.h>
#include <string.h>

size_t convert_hex(uint8_t *dest, size_t max_len, const char *src)
{
    size_t src_len = strlen(src);
    size_t out_len = 0;

    if ((src_len == 0) || ((src_len % 2) != 0))
    {
        return 0;
    }

    for (size_t i = 0; (i < src_len) && (out_len < max_len); i += 2)
    {
        int value;
        if (sscanf(src + i, "%2x", &value) != 1)
        {
            return 0;
        }
        dest[out_len++] = (uint8_t)value;
    }

    if (out_len != (src_len / 2))
    {
        return 0;
    }

    return out_len;
}

void print_hex_payload(const uint8_t *buf, size_t len)
{
    for (size_t i = 0; i < len; i++)
    {
        printf("%02X", buf[i]);
    }
}
