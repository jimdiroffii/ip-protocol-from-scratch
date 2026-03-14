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

size_t build_ip_v4_header(uint8_t *buffer, size_t capacity, const char *src_ip, const char *dst_ip, uint8_t ttl, uint16_t id, uint8_t protocol, size_t payload_len)
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
    ip->type_of_service = 0;        /**< @todo Create enum for TOS values */
    ip->identification = htons(id); // <-- Updated: Dynamic ID
    ip->flags_frag_offset = 0;      /**< @todo Add ability to set flags and fragmentation options */
    ip->ttl = ttl;                  // <-- Updated: Dynamic TTL
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

size_t build_icmp_v4_echo_request(uint8_t *buffer, size_t capacity, uint8_t type, uint8_t code, uint16_t id, uint16_t seq, const char *payload, size_t payload_len)
{
    // The total header is the base 4 bytes + the echo 4 bytes
    size_t header_len = sizeof(struct icmp_v4_header) + sizeof(struct icmp_v4_echo_header);
    size_t total_len = header_len + payload_len;

    // Strict memory boundary check
    if (!buffer || capacity < total_len)
    {
        return 0; /**< @todo Unique error type */
    }

    // 1. Map the base ICMP header (Type, Code, Checksum)
    struct icmp_v4_header *icmp_base = (struct icmp_v4_header *)buffer;
    memset(icmp_base, 0, sizeof(struct icmp_v4_header));

    icmp_base->type = type;
    icmp_base->code = code;
    icmp_base->checksum = 0; /**< Must be 0 before calculating */

    // 2. Map the Echo-specific header right after the base header
    struct icmp_v4_echo_header *icmp_echo = (struct icmp_v4_echo_header *)(buffer + sizeof(struct icmp_v4_header));
    memset(icmp_echo, 0, sizeof(struct icmp_v4_echo_header));

    // Convert 16-bit values to Network Byte Order
    icmp_echo->identifier = htons(id);
    icmp_echo->sequence = htons(seq);

    // 3. Copy the payload immediately following the echo header
    if (payload_len > 0 && payload != NULL)
    {
        memcpy(buffer + header_len, payload, payload_len);
    }

    // 4. Calculate the ICMP checksum (covers base header + echo header + payload)
    icmp_base->checksum = compute_checksum(buffer, total_len);

    return total_len;
}