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
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>

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
 * @param buffer_capacity  Total size of packet buffer (currently unused).
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
    iphdr->checksum = htons(0x0000);
    inet_pton(AF_INET, src, &iphdr->src_addr);
    inet_pton(AF_INET, dst, &iphdr->dst_addr);
    iphdr->checksum = 0;
    iphdr->checksum = compute_checksum(iphdr, sizeof(struct ipv4_header));
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
    construct_icmp_data(buffer, seq);
    construct_icmp_payload(buffer, data, data_len);
    construct_icmp_header(buffer, data_len);
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
    uint8_t packet_buffer[PACKET_MAX_SIZE];
    int sock = create_raw_socket();
    if (sock < 0)
    {
        fprintf(stderr, "Error: Socket creation failed.\n");
        return;
    }

    for (int i = 0; i < n; ++i)
    {
        memset(packet_buffer, 0, sizeof(packet_buffer)); // TODO: Don't need to clear the entire buffer
        size_t packet_len = construct_packet(packet_buffer, sizeof(packet_buffer), src, dst, data, data_len, i);

        if (packet_len == 0)
        {
            fprintf(stderr, "Error: Packet could not be constructed\n");
            return;
        }

        // Debug Packet
        printf("\nRaw IPv4 + ICMP Datagram (Wire Format):\n");
        for (size_t i = 0; i < packet_len; ++i)
        {
            printf("%02X ", packet_buffer[i]);
            if ((i + 1) % 4 == 0)
                printf("\n");
        }
        printf("\n");

        ssize_t bytes_sent = send_packet(sock, packet_buffer);

        if (bytes_sent < 0)
        {
            fprintf(stderr, "Error: Failed to send packet.\n");
            return;
        }

        printf("Bytes sent in packet number %i: %li\n", i, bytes_sent);
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
int main(void)
{
    printf("** ICMP Echo Sender **\n");

    char *src_addr = "192.168.50.128";
    // char *src_addr = "127.0.0.1";
    printf("Source Address: %s\n", src_addr);

    char *dst_addr = "192.168.50.230";
    // char *dst_addr = "127.0.0.1";
    printf("Destination Address: %s\n", dst_addr);

    char *payload = "HELLO";
    printf("Payload: %s\n", payload);

    int payload_len = 5; // TODO: dynamic payload sizing
    printf("Payload Length: %i\n", payload_len);

    // TODO: When implementing dynamic payload sizes from user input, it must
    //       be verified that the payload requested fits in the buffer.
    //       The below conditional will always result in false at the moment,
    //       but has been set as a reminder for future versions.
    if (payload_len > PAYLOAD_MAX_SIZE)
    {
        fprintf(stderr, "Payload exceeds maximum size of: %i\n", PAYLOAD_MAX_SIZE);
        return -1;
    }

    int num_to_send = 3;
    printf("Number of packets: %i\n", num_to_send);

    execute(src_addr, dst_addr, payload, payload_len, num_to_send);

    return 0;
}
