/***
 * @file icmpv4.h
 * @brief RFC implementation of the ICMP protocol
 *
 * @note ICMP is the historical name. ICMPv4 is a retronym. Modified to make it
 * distinct from ICMPv6.
 *
 * ICMP: RFC 792
 *
 * @todo Add additional ICMP message types and fields (i.e. Destination Unreachable)
 */
#ifndef ICMPV4_H
#define ICMPV4_H

#include <stdint.h>

/**
 * @brief Standard code for Echo Request/Reply.
 * @note May be overridden by the user for security auditing (e.g., ICMP tunneling).
 */
#define ICMP_V4_ECHO_CODE 0

/**
 * @enum icmp_v4_message_type
 * @brief ICMPv4 message type field identifiers
 */
enum icmp_v4_message_type
{
    ICMPV4_ECHO_REPLY = 0,
    ICMPV4_ECHO = 8
};

/**
 * @struct icmp_v4_header
 * @brief ICMPv4 header (type/code/checksum), packed wire layout. Required for all ICMPv4 packets.
 */
struct icmp_v4_header
{
    uint8_t type;      /**< Message type */
    uint8_t code;      /**< Message code */
    uint16_t checksum; /**< ICMP checksum of ICMP header + ICMP data (ones' complement) */
} __attribute__((packed));

/**
 * @struct icmp_v4_echo
 * @brief ICMPv4 Echo message fixed fields, packed wire layout.
 */
struct icmp_v4_echo
{
    uint16_t id;       /**< Session identifier */
    uint16_t sequence; /**< Sequence number */
} __attribute__((packed));

#endif /* ICMPV4_H */