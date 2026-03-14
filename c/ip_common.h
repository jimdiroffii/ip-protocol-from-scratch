/**
 * @file ip_common.h
 *
 * @brief Shared components and parameters for IPv4 and IPv6 protocols
 * @see RFC 791
 * @see RFC 8200
 *
 * @author Jim Diroff II
 */

#ifndef IP_COMMON_H
#define IP_COMMON_H

enum ip_version
{
    IP_V4 = 4,
    IP_V6 = 6
};

enum ip_protocol
{
    IP_PROTO_ICMP_V4 = 1,
    IP_PROTO_ICMP_V6 = 58
};

#endif /* IP_COMMON_H */