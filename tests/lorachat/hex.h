#ifndef HEX_H
#define HEX_H

#include <stdint.h>
#include <stddef.h>

size_t convert_hex(uint8_t *dest, size_t max_len, const char *src);
void print_hex_payload(const uint8_t *buf, size_t len);

#endif
