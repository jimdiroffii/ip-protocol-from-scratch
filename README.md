# IP Protocol from Scratch

This is an effort to fully understand IPv4 and IPv6 by building up the protocols from scratch and using them.

## Version 1

The first version if the IP Header and ICMP Header/Payload was built in C. The structure of the program, including comments, is extremely verbose. Usage of `assert` is heavy, and *over*validates the data structures as they are built up. This code was built for purely educational purposes to practice the bit manipulation and other techniques required to construct a custom IP packet and send it out "over the wire."

Compile:

```bash
gcc -Wall -Wextra ip_stack_v1.c -o ip_stack_v1; sudo ./ip_stack_v1
```

Output:

```bash
Test 1 Passed: ipv4_header offsets are correctly positioned.
Test 2 Passed: ipv4_header is exactly 20 bytes.
Test 3 Passed: version_ihl packed correctly (0x45)
Test 4 Passed: Type of Service and Total Length are correct.
Test 5 Passed: Exactly 4 contiguous bytes have been populated.
Test 6 Passed: Identification, Flags and Fragment Offset are correct.
Test 7 Passed: Exactly 8 contiguous bytes have been populated.
Test 8 Passed: TTL, Protocol, and Checksum correctly set
Test 9 Passed: Exactly 12 contiguous bytes have been populated.
Test 10 Passed: SRC and DST correctly set to 127.0.0.1
Test 11 Passed: Exactly 20 contiguous bytes have been populated.
Header complete.
Test 12 Passed: Checksum validation with dummy data.
Test 13 Passed: Checksum validation with constructed packet header.

Raw IPv4 Header (Wire Format):
45 00 00 14 
BE EF 00 00 
40 01 BD F7 
7F 00 00 01 
7F 00 00 01 

Raw IPv4 Header (Wire Format) | Packet 2:
45 00 00 14 
BE EF 00 00 
40 01 BD F7 
7F 00 00 01 
7F 00 00 01 

Test 14 Passed: ICMP header set and offset is exactly 20 bytes.
New IPv4 Header Checksum: F3BD
Test 15 Passed: Total length updated and checksum recalculated.
Test 16 Passed: ICMP Echo specific fields mapped precisely at offset 24.
New IPv4 Header Checksum: EFBD
Test 17 Passed: Total length updated and checksum recalculated.
Test 18 Passed: ICMP Checksum calculated as E5CA
Test 19 Passed: Checksum algorithm now handles odd-byte payloads.
Test 20 Passed: Payload attached. IP Total Length: 33. Checksums updated.

Raw IPv4 + ICMP Datagram (Wire Format):
45 00 00 21 
BE EF 00 00 
40 01 BD EA 
7F 00 00 01 
7F 00 00 01 
08 00 02 39 
12 34 00 01 
48 45 4C 4C 
4F 
Phase 4: Raw socket successfully created with IP_HDRINCL (fd: 3).
Test 21 Passed: Kernel routing structure (sockaddr_in) populated.
Test 22 Passed: Successfully injected 33 bytes onto the wire.
Use `sudo tcpdump -vv -i lo -n icmp` to see the results.
Listening for Echo Reply...
Test 25 Passed: Ignored our own request and caught the true Echo Reply!
Test 26 Passed: Echo Reply Identifier and Sequence match our Request.
Test 27 Passed: Payload successfully extracted and verified as 'HELLO'.
```
