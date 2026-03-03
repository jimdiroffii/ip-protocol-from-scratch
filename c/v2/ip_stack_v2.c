/***
 * @file ip_icmp_stack_v2.c
 * @brief Minimal IPv4 + ICMP packet construction (learning exercise).
 *
 * IP / ICMP Stack V2
 *
 * This program is built using the lessons from V1, but completely refactored
 * to be more dynamic.
 *
 * @note This source is intended for educational purposes and currently only
 *       constructs headers (sending/receiving is not implemented here).
 */

#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <sys/time.h>
#include <getopt.h>
#include <errno.h>

/**
 * @def PACKET_MAX_SIZE
 * @brief Maximum supported packet buffer size (bytes).
 */
#define PACKET_MAX_SIZE 65535

/**
 * @def PAYLOAD_MAX_SIZE
 * @brief Maximum supported payload size (bytes).
 *        PACKET_MAX_SIZE - (20 byte IP header + 4 byte ICMP header + 4 byte ICMP data)
 */
#define PAYLOAD_MAX_SIZE 65507

/**
 * @def IP_HEADER_LENGTH
 * @brief IPv4 IHL value in 32-bit words for a standard 20-byte header.
 */
#define IP_HEADER_LENGTH 5

/**
 * @def STANDARD_TTL
 * @brief Default IPv4 Time-To-Live used for constructed packets.
 */
#define STANDARD_TTL 64

/**
 * @enum ip_version
 * @brief IP protocol version identifiers used in the IPv4 header.
 */
enum ip_version
{

    IPV4 = 4, /** IPv4 */
    IPV6 = 6  /** IPv6 */
};

/**
 * @enum ip_protocol
 * @brief Layer-4 protocol identifiers for the IPv4 header "protocol" field.
 */
enum ip_protocol
{
    /** ICMP (Internet Control Message Protocol) */
    IP_PROTO_ICMP = 1
};

/**
 * @enum icmp_message_type
 * @brief ICMP message "type" field identifiers.
 */
enum icmp_message_type
{
    /** Echo Reply (ping response) */
    ECHO_REPLY = 0,
    /** Echo Request (ping request) */
    ECHO = 8
};

/**
 * @struct ipv4_header
 * @brief IPv4 header (RFC 791), packed wire layout.
 *
 * @note This struct models the 20-byte IPv4 header without options (IHL=5).
 *       Multi-byte fields are typically stored in network byte order when
 *       serialized on the wire.
 */
struct ipv4_header
{
    uint8_t version_ihl;        /** IP Version (4 bits) + Internet Header Length (4 bits). */
    uint8_t type_of_service;    /** Type of Service / DSCP+ECN (QoS). */
    uint16_t total_length;      /** Total length of the IPv4 packet (header + payload), in bytes. */
    uint16_t identification;    /** Identification value for fragmentation/reassembly. */
    uint16_t flags_frag_offset; /** Flags (3 bits) + Fragment Offset (13 bits). */
    uint8_t time_to_live;       /** Time To Live (TTL). */
    uint8_t protocol;           /** Encapsulated L4 protocol (e.g., ICMP=1). */
    uint16_t checksum;          /** IPv4 header checksum (ones' complement). */
    uint32_t src_addr;          /** Source IPv4 address (network order). */
    uint32_t dst_addr;          /** Destination IPv4 address (network order). */
} __attribute__((packed));

/**
 * @struct icmp_header
 * @brief ICMP header (type/code/checksum), packed wire layout. Required for all ICMP packets.
 *
 * @note For many ICMP messages, additional fields follow this header (e.g.,
 *       echo identifier/sequence).
 */
struct icmp_header
{

    uint8_t type;      /** ICMP message type (e.g., Echo=8, Echo Reply=0). */
    uint8_t code;      /** ICMP message code (meaning depends on @ref type). */
    uint16_t checksum; /** ICMP checksum over ICMP header + ICMP data (ones' complement). */
} __attribute__((packed));

/***
 * @struct icmp_echo
 * @brief ICMP Echo message fixed fields (identifier + sequence), packed.
 *
 * Required header information for an echo / echo reply request. Not payload data.
 *
 * @field identifier  Session ID, may be 0
 * @field sequence    Sequence number, may be 0
 */
struct icmp_echo
{
    uint16_t identifier; /** Session identifier (often used to match requests/replies). */
    uint16_t sequence;   /** Sequence number (often incremented per request). */
} __attribute__((packed));

/**
 * @brief Computes a ones' complement checksum over an arbitrary byte buffer.
 *
 * Important: Ensure checksum field is zeroed before computing new checksum
 * Important: htons is unnecessary with checksum, as algorithm is byte-order agnostic
 *
 * This computes the Internet checksum commonly used by IPv4 and ICMP:
 * - Interprets the buffer as 16-bit words
 * - Sums them using 32-bit accumulation
 * - Folds carries into 16 bits
 * - Returns the ones' complement of the final 16-bit sum
 *
 * @param data    Pointer to the data to checksum.
 * @param length  Number of bytes in @p data.
 * @return        16-bit ones' complement checksum.
 */
uint16_t compute_checksum(const void *data, size_t length)
{
    const uint16_t *word_ptr = (const uint16_t *)data;
    uint32_t sum = 0;

    while (length > 1)
    {
        sum += *word_ptr++;
        length -= 2;
    }

    if (length == 1)
    {
        uint16_t padded_word = 0;
        ((uint8_t *)&padded_word)[0] = *(const uint8_t *)word_ptr;
        sum += padded_word;
    }

    while (sum >> 16)
    {
        sum = (sum & 0xFFFF) + (sum >> 16);
    }

    return (uint16_t)~sum;
}

/**
 * @brief Populates an IPv4 header in the provided packet buffer.
 *
 * This writes a minimal 20-byte IPv4 header (no options) at the start of the
 * packet buffer. It sets:
 * - version/IHL
 * - TOS
 * - total length (ipv4 hdr + icmp hdr + icmp echo data + payload)
 * - identification (currently hard-coded)
 * - flags/fragment offset
 * - TTL
 * - protocol (ICMP)
 * - checksum (computed over the header)
 * - source/destination addresses
 *
 * @param buffer           Pointer to packet buffer to populate.
 * @param src              IPv4 source address string.
 * @param dst              IPv4 destination address string.
 * @param data_len         Packet payload string length.
 * @return                 void
 */
void construct_ipv4_header(uint8_t *buffer, const char *src, const char *dst, int data_len)
{
    struct ipv4_header *iphdr = (struct ipv4_header *)buffer;
    uint8_t version = IPV4;
    iphdr->version_ihl = (version << 4) | IP_HEADER_LENGTH;
    iphdr->type_of_service = 0x00;
    iphdr->total_length = htons(sizeof(struct ipv4_header) +
                                sizeof(struct icmp_header) +
                                sizeof(struct icmp_echo) +
                                data_len);
    iphdr->identification = htons(0xBEEF); // BEEF == 48879
    iphdr->flags_frag_offset = htons(0x0000);
    iphdr->time_to_live = STANDARD_TTL;
    iphdr->protocol = IP_PROTO_ICMP;
    inet_pton(AF_INET, src, &iphdr->src_addr);
    inet_pton(AF_INET, dst, &iphdr->dst_addr);
    iphdr->checksum = 0;
    iphdr->checksum = compute_checksum(iphdr, sizeof(struct ipv4_header));
    return;
}

void construct_icmp_data(uint8_t *buffer, int seq)
{
    struct icmp_echo *icmpdata = (struct icmp_echo *)(buffer + (sizeof(struct ipv4_header)) + sizeof(struct icmp_header));
    icmpdata->identifier = htons(0x1234);
    icmpdata->sequence = htons(seq);

    return;
}

void construct_icmp_payload(uint8_t *buffer, const char *data, int len)
{
    uint8_t *payload_ptr = buffer + sizeof(struct ipv4_header) + sizeof(struct icmp_header) + sizeof(struct icmp_echo);
    memcpy(payload_ptr, data, len);
    return;
}

void construct_icmp_header(uint8_t *buffer, int payload_len)
{
    struct icmp_header *icmphdr = (struct icmp_header *)(buffer + sizeof(struct ipv4_header));
    icmphdr->type = ECHO;
    icmphdr->code = 0;
    icmphdr->checksum = 0;
    icmphdr->checksum = compute_checksum(icmphdr, sizeof(struct icmp_header) + sizeof(struct icmp_echo) + payload_len);
    return;
}

/**
 * construct_icmp
 *
 * @brief Specifies the order of ICMP packet build:
 *  1) ICMP Data
 *  2) ICMP Payload
 *  3) ICMP Header (Calculates checksum)
 */
void construct_icmp(uint8_t *buffer, const char *data, int len, int seq)
{
    construct_icmp_data(buffer, seq);
    construct_icmp_payload(buffer, data, len);
    construct_icmp_header(buffer, len);
}

/**
 * @brief Constructs a complete packet into the provided buffer.
 *
 * Currently this function constructs only the IPv4 header (no ICMP header/payload
 * are written yet in this snippet).
 *
 * @param buffer           Pointer to the packet buffer to populate.
 * @param capacity         TODO: Total size of the packet buffer in bytes.
 * @param src              IPv4 source address string (e.g., "192.168.0.10").
 * @param dst              IPv4 destination address string (e.g., "192.168.0.1").
 * @param data             Packet payload string (currently unused in this snippet).
 * @param data_len         Packet paylaod string length.
 * @param seq              Sequence number (currently unused in this snippet).
 * @return                 Size of the packet
 */
size_t construct_packet(uint8_t *buffer, size_t capacity, const char *src, const char *dst, const char *data, int data_len, int seq)
{
    size_t required_size = sizeof(struct ipv4_header) + sizeof(struct icmp_header) + sizeof(struct icmp_echo) + data_len;
    if (required_size > capacity)
    {
        fprintf(stderr, "Error: Required size (%zu) exceeds buffer capacity (%zu)\n", required_size, capacity);
        return 0;
    }
    construct_ipv4_header(buffer, src, dst, data_len);
    construct_icmp(buffer, data, data_len, seq);
    return sizeof(struct ipv4_header) + sizeof(struct icmp_header) + sizeof(struct icmp_echo) + data_len;
}

/**
 * create_raw_socket
 *
 * @brief Creates a raw socket
 *
 * @return A file descriptor (int) to use the socket
 */
int create_raw_socket()
{
    int raw_socket = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
    if (raw_socket < 0)
    {
        fprintf(stderr, "Error: Raw socket creation failed. Are you running as root?\n");
        return -1;
    }
    int hdr_incl = 1;
    if (setsockopt(raw_socket, IPPROTO_IP, IP_HDRINCL, &hdr_incl, sizeof(hdr_incl)) < 0)
    {
        fprintf(stderr, ("Error: Failed to set IP_HDRINCL.\n"));
        return -1;
    }
    return raw_socket;
}

/**
 * send_packet
 *
 * @brief Takes a raw socket and an IPv4 packet buffer, and sends it out on the line
 *
 * @param socket The file descriptor (int) of the socket to use
 * @param buffer The packet buffer containing the headers and payload
 * @return The number of bytes (ssize_t) sent
 */
ssize_t send_packet(int socket, uint8_t *buffer)
{
    struct sockaddr_in dest_info = {0};
    dest_info.sin_family = AF_INET;
    dest_info.sin_port = htons(0);

    struct ipv4_header *iphdr = (struct ipv4_header *)buffer;
    dest_info.sin_addr.s_addr = iphdr->dst_addr;

    ssize_t bytes_sent = sendto(socket, buffer, ntohs(iphdr->total_length), 0, (struct sockaddr *)&dest_info, sizeof(dest_info));

    if (bytes_sent < 0)
    {
        fprintf(stderr, "Error: Failed to send packet.\n");
        close(socket);
        return -1;
    }

    return bytes_sent;
}

ssize_t receive_packet(int socket, uint8_t *buffer, size_t capacity)
{
    struct sockaddr_in sender_info;
    socklen_t sender_len = sizeof(sender_info);
    struct ipv4_header *recv_ip = NULL;
    struct icmp_header *recv_icmp = NULL;
    ssize_t bytes_received;

    while (1)
    {
        bytes_received = recvfrom(socket, buffer, capacity, 0, (struct sockaddr *)&sender_info, &sender_len);
        if (bytes_received < 0)
        {
            if (errno == EINTR)
            {
                continue;
            }
            perror("recvfrom");
            return -1;
        }

        recv_ip = (struct ipv4_header *)buffer;
        uint8_t recv_ihl = recv_ip->version_ihl & 0x0F;
        size_t ip_header_bytes = recv_ihl * 4;
        recv_icmp = (struct icmp_header *)(buffer + ip_header_bytes);
        if (recv_icmp->type == ECHO_REPLY)
        {
            break;
        }
    }

    return bytes_received;
}

/***
 * @brief Generates (constructs) @p n packets.
 *
 * In this snippet, packets are constructed into a local buffer in a loop.
 * Sending/receiving is not implemented here.
 *
 * @param src       IPv4 source address string.
 * @param dst       IPv4 destination address string.
 * @param data      Packet payload string.
 * @param data_len  Packet payload string length.
 * @param n         Number of packets to generate.
 * @return          void
 */
void execute(const char *src, const char *dst, const char *data, int data_len, int n)
{
    uint8_t tx_buffer[PACKET_MAX_SIZE];
    uint8_t rx_buffer[PACKET_MAX_SIZE];
    int sock = create_raw_socket();
    if (sock < 0)
    {
        fprintf(stderr, "Error: Socket creation failed.\n");
        return;
    }

    for (int i = 1; i <= n; ++i)
    {
        memset(tx_buffer, 0, sizeof(tx_buffer)); // TODO: Don't need to clear the entire buffer
        size_t packet_len = construct_packet(tx_buffer, sizeof(tx_buffer), src, dst, data, data_len, i);

        if (packet_len == 0)
        {
            fprintf(stderr, "Error: Packet could not be constructed\n");
            close(sock);
            return;
        }

        // Debug Packet
        // printf("\nRaw IPv4 + ICMP Datagram (Wire Format):\n");
        // for (size_t i = 0; i < packet_len; ++i)
        // {
        //     printf("%02X ", tx_buffer[i]);
        //     if ((i + 1) % 4 == 0)
        //         printf("\n");
        // }
        // printf("\n");

        struct timeval start_time, end_time;
        gettimeofday(&start_time, NULL);

        ssize_t bytes_sent = send_packet(sock, tx_buffer);

        if (bytes_sent < 0)
        {
            fprintf(stderr, "Error: Failed to send packet.\n");
            close(sock);
            return;
        }

        ssize_t bytes_received = receive_packet(sock, rx_buffer, sizeof(rx_buffer));

        gettimeofday(&end_time, NULL);
        double time_ms = (end_time.tv_sec - start_time.tv_sec) * 1000.0;
        time_ms += (end_time.tv_usec - start_time.tv_usec) / 1000.0;

        printf("Bytes sent in packet number %i: %li\n", i, bytes_sent);

        if (bytes_received < 0)
        {
            fprintf(stderr, "Error: Failed to detect echo reply.\n");
        }

        printf("Bytes received: %li, time=%.3f ms\n", bytes_received, time_ms);

        // Debug Packet
        // printf("\nREPLY: Raw IPv4 + ICMP Datagram (Wire Format):\n");
        // for (size_t i = 0; i < packet_len; ++i)
        // {
        //     printf("%02X ", rx_buffer[i]);
        //     if ((i + 1) % 4 == 0)
        //         printf("\n");
        // }
        // printf("\n");
    }

    close(sock);
    return;
}

/**
 * @brief Program entry point.
 *
 * Prints configuration info and invokes @ref execute to generate packets.
 *
 * @return Exit status code (0 on success).
 */
int main(int argc, char *argv[])
{
    printf("** ICMP Echo Sender **\n");

    // 1. Set default values in case the user doesn't provide flags
    char *src_addr = "127.0.0.1";
    char *dst_addr = "127.0.0.1";
    char *payload = "HELLO";
    int num_to_send = 3;

    // 2. Parse the command line arguments
    int opt;
    char *end;
    // "s:d:p:c:" means we expect -s, -d, -p, and -c, and each requires an argument
    while ((opt = getopt(argc, argv, "s:d:p:c:")) != -1)
    {
        switch (opt)
        {
        case 's':
            src_addr = optarg;
            break;
        case 'd':
            dst_addr = optarg;
            break;
        case 'p':
            payload = optarg;
            break;
        case 'c':
            num_to_send = (int)strtol(optarg, &end, 10);
            if (*end != '\0' || num_to_send <= 0)
            {
                fprintf(stderr, "Error: Invalid count '%s'\n", optarg);
                return -1;
            }
            break;
        default:
            fprintf(stderr, "Usage: %s [-s src_ip] [-d dst_ip] [-p payload] [-c count]\n", argv[0]);
            return -1;
        }
    }

    // 3. Dynamically calculate the payload length
    int payload_len = strlen(payload);

    // Print configuration
    printf("Source Address: %s\n", src_addr);
    printf("Destination Address: %s\n", dst_addr);
    printf("Payload: %s (Len: %i)\n", payload, payload_len);
    printf("Number of packets: %i\n\n", num_to_send);

    // 4. Bounds checking
    if (payload_len > PAYLOAD_MAX_SIZE)
    {
        fprintf(stderr, "Error: Payload exceeds maximum size of: %i\n", PAYLOAD_MAX_SIZE);
        return -1;
    }

    // 5. Execute
    execute(src_addr, dst_addr, payload, payload_len, num_to_send);

    return 0;
}
