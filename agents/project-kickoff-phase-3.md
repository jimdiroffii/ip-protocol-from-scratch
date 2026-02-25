Project Kickoff: IP Stack From Scratch (Phase 3 - ICMP Payload)
I. Project Status and Immediate Objectives
We are building a custom, ground-up network stack in C, strictly adhering to RFC specifications.

Summary of Work Performed (Phase 1 & 2):
We have successfully defined, memory-mapped, and initialized the foundational 20-byte Internet Protocol version 4 (IPv4) header (RFC 791). We validated exact memory offsets, handled byte-order translations, and safely packed bit-level fields. In Phase 2, we solved the "chicken and egg" problem of the header_checksum by implementing a highly accurate One's Complement addition algorithm (compute_checksum). We mathematically validated the Accumulate, Fold, and Invert logic, applied it to our live IP header, and successfully printed the exact hex wire-format of our datagram to the terminal.

Next Immediate Objective:
Phase 3 will commence with defining and encapsulating an ICMP (Ping) payload (RFC 792) to attach to our completed IPv4 header. This will involve defining the ICMP struct, managing payload memory correctly, expanding our checksum logic to handle odd-byte lengths, and correctly adjusting the IP header's total_length field.

II. Engineering Philosophy and Strict Working Style
The fundamental law of this project is disciplined, iterative engineering. Depth of conceptual understanding takes absolute precedence over speed. The assisting AI agent must strictly adhere to the following workflow:

Micro-Iterations: Never introduce more than one problem or concept at a time. Break complex structures down into their smallest logical units.

Code Volume Limits: Never provide more than 5 to 10 lines of new code per iteration.

NEVER SKIP AHEAD (The Golden Rule): The user actively analyzes, writes custom comments, and compiles code line-by-line. The agent must never assume the user is ready for the next step. You must wait for explicit confirmation from the user (e.g., "The code compiled", "I am ready to proceed") before providing the next block of code or the next test case.

Explain the "Why": Explain the purpose of every structure, header field, and logic gate before implementation, and validate it mathematically after implementation.

Continuous Regression Testing: Every single field, structure, or mathematical operation must be validated with an assert( ... == ... ) before moving to the next component. Tests are never deleted; they are accumulated to build a robust regression suite.

Deferred Complexity: We build only what the immediate requirements dictate. We intentionally defer handling edge cases (like the IP Options field or odd-byte checksum lengths) until the exact moment our code actually requires them.

III. Technical Decisions and C Programming Paradigms
To ensure absolute memory safety and cross-platform reliability, we have established the following C programming standards:

Memory Layout & Padding: We strictly use the __attribute__((packed)) directive on our protocol structs. Network headers are rigid wire protocols; we must prevent the C compiler from silently injecting padding bytes.

Exact-Width Types: We exclusively use <stdint.h> types (e.g., uint8_t, uint16_t, uint32_t) to guarantee memory footprints.

Bit Manipulation vs. Bitfields: We expressly forbid the use of C bitfields. We manually pack bits using bitwise shifts (<<, >>) and masks (&, |).

Handling Endianness: We strictly enforce Network Byte Order (Big-Endian). Multi-byte fields must be written using htons() or htonl(), and read/tested using ntohs() or ntohl().

Self-Documenting Code: Hardcoded "magic numbers" are forbidden. We use enum to explicitly map out Assigned Numbers (e.g., IP_PROTO_ICMP = 1).

IV. Lessons Learned and Memories to Keep
During Phase 2, we deeply analyzed C memory management and pointer arithmetic. The agent should assume the user understands the following concepts:

Memory is Just Contiguous Bytes: An array or struct in C has no inherent "shape" in physical memory. The pointer type used to access the memory (the "reading glasses") dictates how the bytes are chunked.

The Magic of void *: Passing raw memory as const void * allows us to cleanly cast it to const uint16_t * to walk over the data in 2-byte chunks (essential for the 16-bit One's Complement checksum).

Pointer Synchronization: When using a uint16_t * pointer, incrementing it (ptr++) moves the physical address forward by 2 bytes. Any size_t length counter tracking raw bytes must correspondingly be decremented by 2.

Pointer const Correctness: Reading C pointer declarations right-to-left is critical. const uint16_t *ptr means the data is protected from mutation, but the pointer itself can be moved—perfect for safely walking over a network buffer.

The Danger of Endianness in Arrays: Defining a test payload as uint8_t data[] = { 0xFF, 0x00 } places the bytes in physical memory exactly as written (Network Byte Order). Defining it as uint16_t data[] = { 0xFF00 } causes a Little-Endian CPU to reverse the physical bytes in memory (00 FF), which will silently destroy protocol math. We must always define raw wire data carefully.

V. Agent Interrogation & Clarification
To the assisting AI agent: Before we write the first line of code for Phase 3, review this entire document.

Do you have any questions regarding these strict rules of engagement, particularly the rule regarding "Never Skipping Ahead"?

Do you understand the C paradigms we have adopted and the specific state of our Phase 2 code?

State any ambiguities regarding how we plan to integrate the ICMP payload with our existing IP header now.
