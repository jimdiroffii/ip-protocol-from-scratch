/**
 * @file main.c
 * @brief A packet generator and sender for IPv4 and IPv6 ICMP.
 *
 * @author Jim Diroff II
 */
#include "app_config.h"
#include "icmp_executor.h"
#include "icmp_v4.h"
#include "ip_common.h"
#include "ip_v4.h"

#include <getopt.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#define BASE10 10

/**
 * @brief Safely initializes the application configuration with deterministic defaults.
 * @param config Pointer to the allocated app_config struct.
 */
void init_default_config(struct app_config *config)
{
    memset(config, 0, sizeof(struct app_config));

    config->quantity = 1;
    config->sleep_time = 1;
    config->payload = "HELLO";
    config->payload_len = 5;

    config->ip_v4_src_addr = "127.0.0.1";
    config->ip_v4_dst_addr = "127.0.0.1";

    config->ip_v4_ttl = IP_V4_STD_TTL;
    config->icmp_v4_identifier = 0;
    config->icmp_v4_sequence = 0;

    config->icmp_v4_type = ICMP_V4_ECHO_REQUEST;
    config->icmp_v4_code = ICMP_V4_ECHO_CODE;
}

int main(int argc, char *argv[])
{
    struct app_config config;
    init_default_config(&config);

    /**
     * Arguments:
     *
     * s:src, d:dst, p:payload, c:count, t:ttl, T:type, C:code, i:id, S:sequence, w:wait(sleep_time)
     */
    int opt;
    while ((opt = getopt(argc, argv, "s:d:p:c:t:T:C:i:S:w:")) != -1)
    {
        switch (opt)
        {
        case 's':
            config.ip_v4_src_addr = optarg;
            break;
        case 'd':
            config.ip_v4_dst_addr = optarg;
            break;
        case 'p':
            config.payload = optarg;
            config.payload_len = strlen(config.payload);
            break;
        case 'c':
        {
            char *endptr;
            long val = strtol(optarg, &endptr, BASE10);
            if (*endptr != '\0' || val <= 0)
            {
                fprintf(stderr, "Error: Invalid quantity '%s'\n", optarg);
                return -1;
            }
            config.quantity = (uint32_t)val;
            break;
        }
        case 't':
        {
            char *endptr;
            long val = strtol(optarg, &endptr, BASE10);
            if (*endptr != '\0' || val < 0 || val > UINT8_MAX)
            {
                fprintf(stderr, "Error: Invalid TTL '%s'. Must be 0-%i.\n", optarg, UINT8_MAX);
                return -1;
            }
            config.ip_v4_ttl = (uint8_t)val;
            break;
        }
        case 'T':
        {
            char *endptr;
            long val = strtol(optarg, &endptr, BASE10);
            if (*endptr != '\0' || val != ICMP_V4_ECHO_REQUEST)
            {
                fprintf(stderr, "Error: Invalid type '%s'. Only Echo Request is implemented (%i)\n", optarg, ICMP_V4_ECHO_REQUEST);
                return -1;
            }
            config.icmp_v4_type = (uint8_t)val;
            break;
        }
        case 'C':
        {
            char *endptr;
            long val = strtol(optarg, &endptr, BASE10);
            if (*endptr != '\0' || val < 0 || val > UINT8_MAX)
            {
                fprintf(stderr, "Error: Invalid code '%s'. Must be 0-%i\n", optarg, UINT8_MAX);
                return -1;
            }
            config.icmp_v4_code = (uint8_t)val;
            break;
        }
        case 'i':
        {
            char *endptr;
            long val = strtol(optarg, &endptr, BASE10);
            if (*endptr != '\0' || val < 0 || val > UINT16_MAX)
            {
                fprintf(stderr, "Error: Invalid identifier '%s'. Must be 0-%i\n", optarg, UINT16_MAX);
                return -1;
            }
            config.icmp_v4_identifier = (uint16_t)val;
            break;
        }
        case 'S':
        {
            char *endptr;
            long val = strtol(optarg, &endptr, BASE10);
            if (*endptr != '\0' || val < 0 || val > UINT16_MAX)
            {
                fprintf(stderr, "Error: Invalid sequence '%s'. Must be 0-%i\n", optarg, UINT16_MAX);
                return -1;
            }
            config.icmp_v4_sequence = (uint16_t)val;
            break;
        }
        case 'w':
        {
            char *endptr;
            long val = strtol(optarg, &endptr, BASE10);
            if (*endptr != '\0' || val < 0 || val > UINT8_MAX)
            {
                fprintf(stderr, "Error: Invalid wait (sleep) time '%s'. Must be 0-%i\n", optarg, UINT8_MAX);
                return -1;
            }
            config.sleep_time = (uint8_t)val;
            break;
        }
        default:
            fprintf(stderr, "Usage: %s [-s src_ip] [-d dst_ip] [-p payload] [-c count] "
                            "[-t ttl] [-T type] [-C code] [-i id] [-S sequence] [-w wait]\n",
                    argv[0]);
            return -1;
        }
    }

    printf("\n** ICMP Protocol From Scratch **\n");
    printf("--------------------------------------------------\n");
    printf("[Execution]     %u packet(s), %u second wait\n", config.quantity, config.sleep_time);
    printf("[IPv4]          Src: %s -> Dst: %s | TTL: %u\n", config.ip_v4_src_addr, config.ip_v4_dst_addr, config.ip_v4_ttl);
    printf("[ICMPv4]        Type: %u | Code: %u | ID: %u | Seq: %u\n", config.icmp_v4_type, config.icmp_v4_code, config.icmp_v4_identifier, config.icmp_v4_sequence);
    printf("[Payload Size]  %zu bytes\n", config.payload_len);
    printf("[Payload]       %s\n", config.payload);
    printf("--------------------------------------------------\n\n");

    /** Execute */
    for (uint32_t i = 0; i < config.quantity; i++)
    {
        /** Sequence is allowed to overflow back to `0` */
        uint16_t current_seq = (uint16_t)(config.icmp_v4_sequence + i);
        if (send_icmp_echo(&config, current_seq) != 0)
        {
            fprintf(stderr, "Error: The packet transmission failed at packet %i\n", i);
            return -1;
        }

        if (i < config.quantity - 1)
        {
            sleep(config.sleep_time);
        }
    }

    return 0;
}