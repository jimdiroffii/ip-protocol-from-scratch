#include <stdio.h>
#include <stdint.h>    // fixed type sizes (uint8_t, etc)
#include <assert.h>    // turn assumptions into assertions
#include <stddef.h>    // offsetof()
#include <arpa/inet.h> // htons(), ntohs(), inet_pton(), AF_INET

// Layer 4 Protocol Identifiers
enum ip_protocol {
	IP_PROTO_ICMP = 1,
	IP_PROTO_TCP  = 6,
	IP_PROTO_UDP  = 17
};

// The IPv4 Header, modeled from RFC 791
// The header is laid out in 32-bit (4-byte) words
// To guarantee the compiler does not add any padding to align the struct
// to memory boundries, the __attribute__((packed)) directive is added
struct __attribute__((packed)) ipv4_header {
	// First 32 bits of an IPv4 Header
	uint8_t  version_ihl;      /* 8 bits: Version (4) + Internet Header Length (4) */
	uint8_t  type_of_service;  /* 8 bits: Type of Service */
	uint16_t total_length;     /* 16 bits: Total Length */

	// Second 32 bits contain information regarding packet fragmentation
	// `flags_frag_offset` uses 16 bits to represent two different fields
	// These fields are modified using bitwise shifting masking
	// Identification: Assigned by sender to assist in reassembly of fragments
	// Flags: Bit 0 - (Reserved), Bit 1 - Don't Fragment (DF), Bit 2 - More Fragments (MF)
	// Fragment Offset: The location in the datagram where this fragment belongs
	uint16_t identification;    /* 16 bits: Identification */
	uint16_t flags_frag_offset; /* 16 bits: Flags (3) + Fragment Offset (13) */

	// Third 32 bits contain information on the lifetime, layer 4 protocol, and an integrity check
	// Time to Live (TTL): Originally designed for seconds, this de facto became number of hops
	// If TTL hits zero, the datagram is destroyed
	// Protocol: Layer 4 protocol in use: TCP, UDP or ICMP
	// Header Checksum: Checksum of the IP header only, verified at every hop
	uint8_t  time_to_live;    /* 8 bits: Time to Live (TTL) */
	uint8_t  protocol;        /* 8 bits: Protocol */
	uint16_t header_checksum; /* 16 bits: Header Checksum */

	// Fourth and Fifth sets of 32 bits contain the addressing for source and destination
	// These are the typical IPv4 addresses we are used to: 111.111.111.111
	// Each octet ranges from 0 to 255
	uint32_t src_addr;  /* 32 bits: Source Address */
	uint32_t dst_addr;  /* 32 bits: Destination Address */
};

/***
 * compute_checksum()
 *
 * This function takes an IP header, and calculates the checksum using one's complement addition.
 */
uint16_t compute_checksum(const void *data, size_t length) {
	// Declare a 16-bit pointer that will walk over the IP packet, 16-bits at a time
	const uint16_t *word_ptr = (const uint16_t *)data;
	// The sum must be a 32-bit value, to handle one's complement addition and the carry bits
	uint32_t sum = 0;

	while (length > 1) {
		sum += *word_ptr++; /* add the 16-bits of data to the sum and increment the pointer */
		length -= 2;        /* we moved 2 bytes forward, so we must decrement by 2 */
	}

	// Accumulate and Fold (carry)
	// `sum >> 16` is a bitwise shift that pulls the top 16 bits down to the bottom, isolating the carries
	// `sum & 0xFFFF` is a bitwise AND which isolates the bottom 16 bits, masking out the top
	// The top 16 and the bottom 16 are added together and stored in `sum`
	// If there are no carries, the shift evaluates to 0, and the loop breaks
	while (sum >> 16) {
		sum = (sum & 0xFFFF) + (sum >> 16);
	}

	// Invert
	// Finally, `~sum` is a bitwise NOT operation that inverts every bit, completing the checksum calculation
	return (uint16_t)~sum;
}

int main(void) {
	// Test 1: Verify the offsets of individual fields by byte order
	assert(offsetof(struct ipv4_header, version_ihl)        == 0);
	assert(offsetof(struct ipv4_header, type_of_service)    == 1);
	assert(offsetof(struct ipv4_header, total_length)       == 2);
	assert(offsetof(struct ipv4_header, identification)     == 4);
	assert(offsetof(struct ipv4_header, flags_frag_offset)  == 6);
	assert(offsetof(struct ipv4_header, time_to_live)       == 8);
	assert(offsetof(struct ipv4_header, protocol)           == 9);
	assert(offsetof(struct ipv4_header, header_checksum)    == 10);
	assert(offsetof(struct ipv4_header, src_addr)           == 12);
	assert(offsetof(struct ipv4_header, dst_addr)           == 16);
	printf("Test 1 Passed: ipv4_header offsets are correctly positioned.\n");

	// Test 2: Verify the total size of the struct
	assert(sizeof(struct ipv4_header) == 20);
	printf("Test 2 Passed: ipv4_header is exactly 20 bytes.\n");

	// The packet is valid, and construction can begin
	struct ipv4_header packet = {0};

	/***
	 * First 32 bits 
	 */

	// Set Version (4) and IHL (5)
	// The version is simply the IP version we are using
	// The Internet Header Length is based on the size of the IP header in 32-bit (4-byte) words
	// The header is exactly 20 bytes
	// 20 / 4 = 5
	// 5 is the minimum value for a correctly formatted IPv4 header
	// Additionally, version and IHL are sharing the same byte.
	// Therefore, the version must be packed into the high-order (left) bits,
	// and IHL into the low-order (right) bits
	// Take the version number (4), and shift it left by 4 bits using the left-shift operator (<<)
	// IHL is applied by using the OR operator (|)
	uint8_t version = 4;
	uint8_t ihl = 5;
	packet.version_ihl = (version << 4) | ihl;

	// Test 3: Verify the first byte using hexidecimal
	assert(packet.version_ihl == 0x45);
	printf("Test 3 Passed: version_ihl packed correctly (0x45)\n");

	// Set the Type of Service. This is used for things like Quality of Service.
	// Our Type of Service will be "routine" or `00`
	packet.type_of_service = 0x00;

	// Set the Total Length (20 byte header + 0 byte payload)
	// An important point here is that networks always use Big-Endian.
	// This is also known as Network Byte Order.
	// Big-Endian means the most significant octet is transmitted first.
	// However, many computers (such as x86), use Little-Endian
	// Also known as Host Byte Order
	// It is critical that network packets are sent in Network Byte Order
	// htons() is used to guarantee this: Host To Network Short = htons
	// Additionally, htons only matters when a piece of data spans multiple bytes.
	// Since total_length is 16 bits, or 2 bytes, the order must be strictly maintained.
	// This is why htons() is not required on single byte values, such as type_of_service.
	packet.total_length = htons(20);

	// Test 4: Verify TOS and Total Length are correctly populated
	assert(packet.type_of_service == 0x00);
	assert(ntohs(packet.total_length) == 20);
	printf("Test 4 Passed: Type of Service and Total Length are correct.\n");
	
	// Test 5: Verify that our data is exactly the size we expect using pointer arithmetic
	size_t populated_bytes = (uint8_t *)&packet.identification - (uint8_t *)&packet;
	assert(populated_bytes == 4);
	printf("Test 5 Passed: Exactly 4 contiguous bytes have been populated.\n");

	/***
	 * Second 32 bits
	 */

	// The fragmentation headers assist with fragmenting packets that are too large
	// to fit on a network segment.
	// The Identification field is assigned by the sender and must be unique to this
	// source-destination-protocol combination for the time the datagram is active
	// We set this test packet to an arbitrary value between 0000 to FFFF
	packet.identification = htons(0xBEEF);

	// The control flags have 3 values: 
	// First bit: Reserved, must be 0
	// Second bit: DF flag, 0 = May Fragment, 1 = Don't Fragment
	// Third bit: MF flag, 0 = Last Fragment, 1 = More Fragments
	// The Fragment Offset shares 13 of the 16 bits allocated here, the other 3 are the flags
	// The fragment offset indicates where in the datagram this fragment belongs
	// It is measured in units of 8 octets (64 bits)
	// The fragment offset is a brilliant bit of binary engineering.
	// The Total Length field is 16 bits. This gives a maximum size of a datagram of 65,535 bytes.
	// The Fragment Offset doesn't point to a specific set of bytes, it points to a block of 8 bytes.
	// If the datagram is fragmented, it must be done on an 8 octet boundary.
	// When reassembled, the network device takes the offset and multiplies it by 8 (bitwise left shift 3)
	// This allows 2^13 = 8192 fragments of 8 octets each for a total of 65,536 octets
	// One more than the maximum allowable size of a datagram
	packet.flags_frag_offset = htons(0x0000);

	assert(ntohs(packet.identification) == 0xBEEF);
	assert(ntohs(packet.flags_frag_offset) == 0x0000);
	printf("Test 6 Passed: Identification, Flags and Fragment Offset are correct.\n");

	populated_bytes = (uint8_t *)&packet.time_to_live - (uint8_t *)&packet;
	assert(populated_bytes == 8);
	printf("Test 7 Passed: Exactly 8 contiguous bytes have been populated.\n");

	/***
	 * Third 32 bits
	 */
	// The Time To Live (TTL) of the packet is essentially a hop count, as every
	// intermediary device the datagram hits decrements the TTL by one. When the TTL
	// is zero, the datagram is destroyed. 64 is the default value.
	packet.time_to_live = 64;

	// The protocol determines the next level protocol in use for this datagram.
	// ICMP = 1, TCP = 6, UDP = 17
	packet.protocol = IP_PROTO_ICMP;

	// Header checksum validate that the datagram is not corrupted in transit. 
	// The values of the header change at every hop, so the checksum is constantly
	// being recalculated. Interestingly, the checksum is part of the header, so 
	// there is a "chicken and egg" problem. The specification states that for the 
	// purposes of the computing the checksum, the checksum field is zero.
	packet.header_checksum = htons(0x0000);

	assert(packet.time_to_live == 64);
	assert(packet.protocol == IP_PROTO_ICMP);
	assert(ntohs(packet.header_checksum) == 0x0000);
	printf("Test 8 Passed: TTL, Protocol, and Checksum correctly set\n");

	populated_bytes = (uint8_t *)&packet.src_addr - (uint8_t *)&packet;
	assert(populated_bytes == 12);
	printf("Test 9 Passed: Exactly 12 contiguous bytes have been populated.\n");

	/***
	 * Fourth and Fifth 32 bits
	 */
	// inet_pton (Pointer to Network) converts a string IP to a network byte order
	// 32-bit integer
	// Valid range for IPv4 is 0.0.0.0 to 255.255.255.255
	// AF_INET specifies that we are working with IPv4 addresses
	inet_pton(AF_INET, "127.0.0.1", &packet.src_addr);
	inet_pton(AF_INET, "127.0.0.1", &packet.dst_addr);

	// 127.0.0.1 in hex: 0x7F (127), 0x00 (0), 0x00 (0), 0x01 (1)
	// ntohl is Network to Host Long
	assert(ntohl(packet.src_addr) == 0x7F000001);
	assert(ntohl(packet.dst_addr) == 0x7F000001);
	printf("Test 10 Passed: SRC and DST correctly set to 127.0.0.1\n");

	populated_bytes = (uint8_t *)(&packet + 1) - (uint8_t *)&packet;
	assert(populated_bytes == 20);
	printf("Test 11 Passed: Exactly 20 contiguous bytes have been populated.\n");

	printf("Header complete.\n");

	// Validate Checksum with Dummy Data
	uint8_t dummy_data[] = { 0xFF, 0x00, 0x01, 0xFF, 0x00, 0x02 };
	uint16_t dummy_checksum = compute_checksum(dummy_data, sizeof(dummy_data));
	assert(dummy_checksum == htons(0xFEFD));
	printf("Test 12 Passed: Checksum validation with dummy data.\n");

	// Calculate and store packet header checksum
	packet.header_checksum = compute_checksum(&packet, sizeof(packet));
	assert(packet.header_checksum == htons(0xBDF7));
	printf("Test 13 Passed: Checksum validation with constructed packet header.\n");

	// Print the raw byte array of the IPv4 header in hexadecimal
	const uint8_t *raw_bytes = (const uint8_t *)&packet;
	printf("\nRaw IPv4 Header (Wire Format):\n");
	for (size_t i = 0; i < sizeof(packet); ++i) {
		printf("%02X ", raw_bytes[i]);
		if ((i + 1) % 4 == 0) printf("\n");
	}

	return 0;
}

