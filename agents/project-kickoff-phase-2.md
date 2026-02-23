Project Kickoff: IP Stack From Scratch (Phase 2)
I. Project Status and Immediate Objectives
We are building a custom, ground-up network stack in C, strictly adhering to RFC specifications.

Current State: We have successfully defined, memory-mapped, and initialized the foundational 20-byte Internet Protocol version 4 (IPv4) header (RFC 791). We have validated the exact memory offsets, handled byte-order translations, and safely packed bit-level fields.

Next Immediate Objective: Phase 2 will commence with solving the "chicken and egg" problem of the header_checksum. We will implement the mathematical algorithm to compute the checksum over the header, followed by the encapsulation of an ICMP (Ping) payload.

II. Engineering Philosophy and Working Style
The fundamental law of this project is disciplined, iterative engineering. Depth of conceptual understanding takes absolute precedence over speed. The assisting agent must strictly adhere to the following workflow:

Micro-Iterations: Never introduce more than one problem or concept at a time. Break complex structures down into their smallest logical units.

Code Volume Limits: Never provide more than 5 to 10 lines of new code per iteration.

Continuous Regression Testing: Every single field, structure, or mathematical operation must be validated with an assert() before moving to the next component. Tests are never deleted; they are accumulated to build a robust regression suite.

Explain the "Why": Explain the purpose of every structure, header field, and logic gate before implementation, and validate it mathematically after implementation.

III. Technical Decisions and Paradigms
To ensure absolute memory safety and cross-platform reliability, we have established the following C programming standards:

Memory Layout & Padding: We strictly use the __attribute__((packed)) directive on our protocol structs. Network headers are rigid wire protocols; we must prevent the C compiler from silently injecting padding bytes for alignment optimization.

Exact-Width Types: We exclusively use <stdint.h> types (e.g., uint8_t, uint16_t, uint32_t) to guarantee memory footprints across different architectures.

Bit Manipulation vs. Bitfields: We expressly forbid the use of C bitfields (e.g., uint8_t version:4) because compiler bit-packing behavior is heavily environment-dependent. Instead, we manually pack bits using bitwise shifts (<<) and masks (|).

Handling Endianness: We strictly enforce Network Byte Order (Big-Endian). Multi-byte fields must be written using htons() or htonl(), and read/tested using ntohs() or ntohl(). Single-byte fields (uint8_t) do not require conversion.

Self-Documenting Code: Hardcoded "magic numbers" (like protocol identifiers) are forbidden. We use enum to explicitly map out Assigned Numbers (e.g., IP_PROTO_ICMP = 1).

String-to-Network Translation: We utilize inet_pton() alongside the <arpa/inet.h> library for safe, standard-compliant IP string conversions.

IV. Critical Lessons Learned and Pitfalls to Avoid
During Phase 1, we identified and corrected several critical systems programming traps. The agent must remain vigilant against these in the future:

Assignment vs. Evaluation in Tests: A single = inside an assert() silently overwrites memory and forces a false positive. We must strictly verify the use of the equality operator (==).

Allocated vs. Initialized Memory: sizeof() only tells us the allocated footprint of a struct. It does not prove we populated the data. To test initialized memory safely, we use pointer arithmetic.

Pointer Arithmetic Logic: Subtracting a base pointer from an advanced pointer yields the distance in memory. However, to get the distance in exactly bytes, both pointers must be explicitly cast to (uint8_t *) before the subtraction.

Mathematical Scaling: Fields like the 13-bit Fragment Offset do not map 1:1 to bytes. We must remember that network specifications frequently use block scaling (e.g., measuring in 8-octet blocks) to maximize small bit-spaces.

V. Agent Interrogation & Clarification
To the assisting AI agent: Before we write the first line of code for Phase 2, review this document. Do you have any questions regarding these strict rules of engagement, the C paradigms we have adopted, or the mathematical state of our current 20-byte header? State any ambiguities now.
