/**
 * @file ip_v4.h
 *
 * @brief RFC implementation of the IP protocol
 * IPv4: RFC 791
 */

#ifndef IP_V4_H
#define IP_V4_H

#include <stdint.h>
#include "ip_common.h"

/**
 * @brief Absolute max IPv4 datagram length (header + payload) in octets.
 */
#define IP_V4_MAX_PACKET_SIZE 65535

/**
 * @brief Minimum datagram size all hosts must be prepared to accept.
 */
#define IP_V4_RECOMMENDED_MTU 576

/**
 * @brief Minimum valid Internet Header Length in 32-bit words (20 bytes).
 */
#define IP_V4_MIN_IHL 5

/**
 * @struct ip_v4_header
 * @brief 20-byte IPv4 header, packed wire layout, without options.
 *
 * @note RFC 791
 */
struct ip_v4_header
{
    uint8_t version_ihl;        /**< IP Version (4 bits) + Internet Header Length (4 bits) */
    uint8_t type_of_service;    /**< DSCP+ECN - See RFC 2474 (DSCP), RFC 3168 (ECN) and their respective updates */
    uint16_t total_length;      /**< Length of complete datagram, in octets. Absolute Max: 65535, Recommended Max: 576 */
    uint16_t identification;    /**< Aids in reassembly of fragments */
    uint16_t flags_frag_offset; /**< Flags (3 bits) + Fragment Offset (13 bits) */
    uint8_t ttl;                /**< Time to Live a.k.a. hop count */
    uint8_t protocol;           /**< Upper-layer protocol id */
    uint16_t checksum;          /**< Ones' complement checksum of all 16-bit words in the header */
    uint32_t src;               /**< Source address */
    uint32_t dst;               /**< Destination address */
} __attribute__((packed));

#endif /* IP_V4_H */