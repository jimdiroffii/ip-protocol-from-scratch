/**
 * @file app_config.h
 * @brief Unified application state definition.
 */
#ifndef APP_CONFIG_H
#define APP_CONFIG_H

#include <stddef.h>
#include <stdint.h>

/**
 * @struct app_config
 * @brief Unified, read-only configuration state parsed from CLI arguments.
 */
struct app_config
{
    // Execution State
    uint32_t quantity;   /**< Number of packets to send */
    uint8_t sleep_time;  /**< Delay in seconds between packets */
    const char *payload; /**< Pointer to user-defined payload string */
    size_t payload_len;  /**< Explicit byte boundary of the payload */

    // IPv4 Configuration
    const char *ip_v4_src_addr; /**< Source IPv4 address string (e.g., "127.0.0.1") */
    const char *ip_v4_dst_addr; /**< Destination IPv4 address string */
    uint8_t ip_v4_ttl;          /**< IPv4 Time To Live (TTL) */

    // ICMPv4 Configuration
    uint8_t icmp_v4_type;        /**< ICMP message type (e.g., Echo Request) */
    uint8_t icmp_v4_code;        /**< ICMP message code for specific routing nuances */
    uint16_t icmp_v4_identifier; /**< Session ID to map replies to requests */
    uint16_t icmp_v4_sequence;   /**< Incremental counter for packet tracking */
};

#endif // APP_CONFIG_H