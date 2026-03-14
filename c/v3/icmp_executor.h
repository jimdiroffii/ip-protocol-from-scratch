/**
 * @file icmp_executor.h
 * @brief OS-level socket execution and bridging logic for ICMP operations.
 */
#ifndef ICMP_EXECUTOR_H
#define ICMP_EXECUTOR_H

#include "app_config.h"
#include "ip_common.h"
#include <stdint.h>

/**
 * @brief Coordinates the packet builder and raw socket transmission.
 * @param config Pointer to the validated application state.
 * @param current_sequence The dynamically calculated sequence number.
 * @return 0 on success, -1 on socket or transmission failure.
 */
int send_icmp_echo(const struct app_config *config, uint16_t current_sequence);

#endif // ICMP_EXECUTOR_H