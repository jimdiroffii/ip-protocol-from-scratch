/**
 * @file packet_builder.c
 * @brief Memory construction logic for network headers.
 *
 * @author Jim Diroff II
 */

#include "packet_builder.h"
#include "checksum.h"

#include <arpa/inet.h>
#include <string.h>

size_t build_ip_v4_header(uint8_t *buffer, size_t capacity, const char *src_ip, const char *dst_ip, uint8_t protocol, size_t payload_len)
{
    if (!buffer || capacity < sizeof(struct ip_v4_header))
    {
        return 0; /**< @todo Unique error type */
    }

    size_t total_len = sizeof(struct ip_v4_header) + payload_len;
    if (total_len > IP_V4_MAX_PACKET_SIZE)
    {
        return 0; /**< @todo Unique error type */
    }

    struct ip_v4_header *ip = (struct ip_v4_header *)buffer;
    memset(ip, 0, sizeof(struct ip_v4_header));

    ip->version_ihl = (IP_V4 << 4) | IP_V4_MIN_IHL;
    ip->total_length = htons((uint16_t)total_len);
    ip->type_of_service = 0;            /**< @todo Create enum for TOS values */
    ip->identification = htons(0xBEEF); /**< @todo Generate random IDs */
    ip->flags_frag_offset = 0;          /**< @todo Add ability to set flags and fragmentation options */
    ip->ttl = IP_V4_STD_TTL;            /**< @todo Make TTL configurable */
    ip->protocol = protocol;
    ip->checksum = 0; /**< Set checksum to `0` before calculation */

    if (inet_pton(AF_INET, src_ip, &ip->src) != 1)
    {
        return 0; /**< @todo Unique error type */
    }

    if (inet_pton(AF_INET, dst_ip, &ip->dst) != 1)
    {
        return 0; /**< @todo Unique error type */
    }

    ip->checksum = compute_checksum(ip, sizeof(struct ip_v4_header));

    return sizeof(struct ip_v4_header);
}