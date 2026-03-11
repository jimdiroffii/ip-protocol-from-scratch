/**
 * ip_common.h
 *
 * @brief Shared components and parameters for IPv4 and IPv6 protocols
 * IPv4: RFC 791
 * IPv6: RFC 8200
 */

#ifndef IP_COMMON_H
#define IP_COMMON_H

enum ip_version
{
    IPV4 = 4,
    IPV6 = 6
};

enum ip_protocol
{
    IP_PROTO_ICMPV4 = 1,
    IP_PROTO_ICMPV6 = 58
};

#endif /* IP_COMMON_H */