/**
 * @file packet_builder.h
 * @brief todo...
 *
 * @author Jim Diroff II
 */
#ifndef PACKET_BUILDER_H
#define PACKET_BUILDER_H

#include "icmp_v4.h"
#include "ip_v4.h"

#include <stddef.h>
#include <stdint.h>

/**
 * @brief Constructs an IPv4 header in the provided buffer.
 * @param buffer      The memory block where the packet will be built.
 * @param capacity    The absolute maximum size of the buffer (to prevent overflow).
 * @param src_ip      Source IP address as a string (e.g., "192.168.1.1").
 * @param dst_ip      Destination IP address as a string (e.g., "8.8.8.8").
 * @param ttl         Time to Live (TTL) for the IP packet.
 * @param id          Identification field for the IP packet.
 * @param protocol    The Layer 4 protocol ID (e.g., IP_PROTO_ICMP_V4).
 * @param payload_len The size of the payload following this header.
 * @return size_t     The number of bytes written (20), or 0 on error.
 */
size_t build_ip_v4_header(uint8_t *buffer, size_t capacity, const char *src_ip, const char *dst_ip, uint8_t ttl, uint16_t id, uint8_t protocol, size_t payload_len);
/**
 * @brief Constructs an ICMPv4 header and copies the payload into the buffer.
 * @param buffer      The memory block where the packet will be built.
 * @param capacity    The absolute maximum size of the buffer.
 * @param type        ICMP Message Type (e.g., Echo Request).
 * @param code        ICMP Message Code.
 * @param id          Session Identifier.
 * @param seq         Sequence Number.
 * @param payload     Pointer to the payload data.
 * @param payload_len The length of the payload in bytes.
 * @return size_t     The total number of bytes written (Header + Payload), or 0 on error.
 */
size_t build_icmp_v4_echo_request(uint8_t *buffer, size_t capacity, uint8_t type, uint8_t code, uint16_t id, uint16_t seq, const char *payload, size_t payload_len);

#endif /* PACKET_BUILDER_H */