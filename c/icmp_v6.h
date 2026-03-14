/***
 * @file icmp_v6.h
 * @brief RFC implementation of the ICMPv6 protocol
 *
 * @see RFC 4443
 *
 * @todo Address updates in RFC 4884 - Extended ICMP to Support Multi-Part Messages
 *
 * @author Jim Diroff II
 */
#ifndef ICMP_V6_H
#define ICMP_V6_H

#include <stdint.h>

/**
 * @brief Standard code for Echo Request/Reply.
 * @note May be overridden by the user for security auditing (e.g., ICMP tunneling).
 */
#define ICMP_V6_ECHO_CODE 0

enum icmp_v6_message_type
{
    ICMP_V6_ECHO_REQUEST = 128,
    ICMP_V6_ECHO_REPLY = 129
};

/**
 * @struct icmp_v6_header
 */
struct icmp_v6_header
{
    uint8_t type;      /**< Message type. Its value determines the format of the remaining data. */
    uint8_t code;      /**< Depends on message type. Creates an additional level of message granularity. */
    uint16_t checksum; /**< Ones' complement checksum of message and header */
} __attribute__((packed));

/**
 * @struct icmp_v6_echo_header
 * @brief ICMPv6 Echo message fixed fields, packed wire layout.
 *        Used for both Echo Reply and Echo Request messages.
 */
struct icmp_v6_echo_header
{
    uint16_t identifier; /**< An identifier to aid in matching Echo Replies to this Echo Request. May be zero. */
    uint16_t sequence;   /**< A sequence number to aid in matching Echo Replies to this Echo Request. May be zero. */
} __attribute__((packed));

#endif /* ICMP_V6_H */