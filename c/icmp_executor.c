#include "icmp_executor.h"
#include "app_config.h"
#include "packet_builder.h"

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h> // For AF_INET, IPPROTO_ICMP, sockaddr_in
#include <arpa/inet.h>  // For inet_pton

int send_icmp_echo(const struct app_config *config, uint16_t current_sequence)
{
    // 1. Request a Raw IPv4 Socket
    int sockfd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
    if (sockfd < 0)
    {
        perror("Error: Failed to open raw socket (Are you running as root?)");
        return -1;
    }

    // 2. Tell the kernel we are providing our own IPv4 header
    int hincl = 1;
    if (setsockopt(sockfd, IPPROTO_IP, IP_HDRINCL, &hincl, sizeof(hincl)) < 0)
    {
        perror("Error: Failed to set IP_HDRINCL socket option");
        close(sockfd);
        return -1;
    }

    // 3. Prepare the Destination Routing Structure
    struct sockaddr_in dest_addr;
    memset(&dest_addr, 0, sizeof(dest_addr));
    dest_addr.sin_family = AF_INET;

    if (inet_pton(AF_INET, config->ip_v4_dst_addr, &dest_addr.sin_addr) != 1)
    {
        fprintf(stderr, "Error: Invalid destination IP address format: %s\n", config->ip_v4_dst_addr);
        close(sockfd);
        return -1;
    }

    // 4. Allocate the packet buffer (Zeroed out)
    uint8_t packet[IP_V4_MAX_PACKET_SIZE];
    memset(packet, 0, sizeof(packet));

    // 5. Build the ICMPv4 Segment (placed immediately after where the IP header will go)
    size_t ip_hdr_size = sizeof(struct ip_v4_header);
    uint8_t *icmp_start = packet + ip_hdr_size;
    size_t max_icmp_capacity = sizeof(packet) - ip_hdr_size;

    size_t icmp_len = build_icmp_v4_echo_request(
        icmp_start, max_icmp_capacity,
        config->icmp_v4_type, config->icmp_v4_code,
        config->icmp_v4_identifier, current_sequence,
        config->payload, config->payload_len);

    if (icmp_len == 0)
    {
        fprintf(stderr, "Error: Failed to construct ICMP segment.\n");
        close(sockfd);
        return -1;
    }

    // 6. Build the IPv4 Header (placed at the very beginning of the buffer)
    // We use the current_sequence as the IP Identification field as well for tracking
    size_t ip_len = build_ip_v4_header(
        packet, sizeof(packet),
        config->ip_v4_src_addr, config->ip_v4_dst_addr,
        config->ip_v4_ttl, current_sequence,
        IP_PROTO_ICMP_V4, icmp_len);

    if (ip_len == 0)
    {
        fprintf(stderr, "Error: Failed to construct IPv4 header.\n");
        close(sockfd);
        return -1;
    }

    // Total size of the packet to send
    size_t total_packet_len = ip_len + icmp_len;

    // 7. Inject the raw bytes onto the wire
    ssize_t bytes_sent = sendto(sockfd, packet, total_packet_len, 0, (struct sockaddr *)&dest_addr, sizeof(dest_addr));

    if (bytes_sent < 0)
    {
        perror("Error: Failed to send packet");
        close(sockfd);
        return -1;
    }
    else if ((size_t)bytes_sent != total_packet_len)
    {
        fprintf(stderr, "Warning: Only sent %zd out of %zu bytes\n", bytes_sent, total_packet_len);
    }

    // 8. Clean up
    close(sockfd);
    return 0;
}