/***
 * @file checksum.c
 * @brief A ones' complement checksum algorithm
 *
 * @author Jim Diroff II
 */

#include "checksum.h"

uint16_t compute_checksum(const void *data, size_t length)
{
    const uint16_t *word_ptr = (const uint16_t *)data;
    uint32_t sum = 0;

    while (length > 1)
    {
        sum += *word_ptr++;
        length -= 2;
    }

    if (length == 1)
    {
        uint16_t padded_word = 0;
        ((uint8_t *)&padded_word)[0] = *(const uint8_t *)word_ptr;
        sum += padded_word;
    }

    while (sum >> 16)
    {
        sum = (sum & 0xFFFF) + (sum >> 16);
    }

    return (uint16_t)~sum;
}