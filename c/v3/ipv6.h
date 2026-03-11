/**
 * ipv6.h
 *
 * @brief RFC implementation of the IPv6 protocol
 * IPv6: RFC 8200
 */

#ifndef IPV6_H
#define IPV6_H

#include <stdint.h>
#include "ip_common.h"

/**
 * @brief The absolute minimum link MTU required for any IPv6 network.
 */
#define IPV6_MIN_LINK_MTU 1280

/**
 * @brief The minimum fragment reassembly size all IPv6 nodes must support.
 */
#define IPV6_MIN_REASSEMBLY_SIZE 1500

/**
 * @brief The fixed length of the base IPv6 header in octets (40 bytes).
 */
#define IPV6_BASE_HEADER_LENGTH 40

/**
 * @struct ipv6_header
 * @brief 40-byte IPv6 header, packed wire layout, without extensions.
 *
 * @note RFC 8200
 */
struct ipv6_header
{
    uint32_t version_class_flow; /**< IP Version (4 bits) + Traffic Class (8 bits) + Flow Label (20 bits) */
    uint16_t payload_length;     /**< Length of the packet following this header (extension headers + payload) */
    uint8_t next_header;         /**< ID of the type of header following this header */
    uint8_t hop_limit;           /**< Hop limit (TTL in IPv4) */
    uint8_t src[16];             /**< Source address (128-bit) */
    uint8_t dst[16];             /**< Destination address (128-bit) */
} __attribute__((packed));

#endif /* IPV6_H */