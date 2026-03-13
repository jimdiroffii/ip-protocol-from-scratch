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
 * @param protocol    The Layer 4 protocol ID (e.g., IP_PROTO_ICMP_V4).
 * @param payload_len The size of the payload following this header.
 * @return size_t     The number of bytes written (20), or 0 on error.
 */
size_t build_ip_v4_header(uint8_t *buffer, size_t capacity, const char *src_ip, const char *dst_ip, uint8_t protocol, size_t payload_len);

size_t build_icmp_v4_echo_request();

#endif /* PACKET_BUILDER_H */