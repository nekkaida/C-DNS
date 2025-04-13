# DNS Forwarding Server Design Document

## Introduction

This document outlines the architectural design and implementation details of the DNS Forwarding Server. The server is designed to efficiently forward DNS queries to an upstream resolver while supporting both single-question and multi-question DNS queries.

## System Architecture

The DNS Forwarding Server follows a simple, event-driven architecture focused on efficient packet processing:

```
                 ┌─────────────────────────────────────────┐
                 │            DNS Forwarding Server        │
                 │                                         │
┌───────────┐    │ ┌──────────┐  ┌──────────┐  ┌─────────┐ │    ┌────────────┐
│           │    │ │          │  │          │  │         │ │    │            │
│  Client   │◄───┼─┤  Socket  │◄─┤  Packet  │◄─┤ Query   │ │◄───┤  Upstream  │
│  (DNS     │    │ │  Handler │  │ Processor│  │ Forwarder│ │    │  Resolver  │
│  Resolver)│    │ │          │  │          │  │         │ │    │            │
│           │────┼─┤          ├──┤          ├──┤         ├─┼────┤            │
└───────────┘    │ └──────────┘  └──────────┘  └─────────┘ │    └────────────┘
                 │                                         │
                 └─────────────────────────────────────────┘
```

### Key Components

1. **Socket Handler**
   - Manages UDP socket communications
   - Receives incoming DNS queries
   - Sends responses back to clients

2. **Packet Processor**
   - Parses DNS packet headers and data
   - Handles domain name compression/decompression
   - Constructs response packets

3. **Query Forwarder**
   - Forwards queries to the upstream resolver
   - Processes resolver responses
   - Handles multi-question queries

## DNS Protocol Implementation

### DNS Packet Structure

The DNS packet format follows RFC 1035:

```
    +---------------------+
    |        Header       |
    +---------------------+
    |       Question      | the question for the name server
    +---------------------+
    |        Answer       | RRs answering the question
    +---------------------+
    |      Authority      | RRs pointing toward an authority
    +---------------------+
    |      Additional     | RRs holding additional information
    +---------------------+
```

### Header Structure

The DNS header is implemented as a packed structure to ensure proper byte alignment:

```c
typedef struct {
    uint16_t id;       // ID field
    uint16_t flags;    // DNS flags
    uint16_t qdcount;  // Question count
    uint16_t ancount;  // Answer count
    uint16_t nscount;  // Authority count
    uint16_t arcount;  // Additional count
} __attribute__((packed)) dns_header_t;
```

### Domain Name Handling

Domain names in DNS use a special format where each label is prefixed with its length:

```
Example: "example.com"
Format:  [7]example[3]com[0]
Bytes:   0x07 e x a m p l e 0x03 c o m 0x00
```

The server includes specialized functions to handle domain name parsing, including support for DNS name compression where pointers (indicated by the two high bits set in a length byte) refer to previously occurring domain name sections.

## Query Processing Logic

### Single-Question Queries

For queries containing a single question, the server:

1. Validates the incoming DNS packet format
2. Forwards the entire packet to the upstream resolver
3. Returns the resolver's response directly to the client

This approach is efficient for typical DNS usage where most queries contain a single question.

### Multi-Question Queries

For queries containing multiple questions, the server:

1. Parses all questions from the incoming query
2. Constructs a response including:
   - All original questions (decompressed)
   - An answer for each question (using A records)
3. Returns the combined response to the client

## Performance Considerations

### Memory Management

The server uses fixed-size buffers (512 bytes) to align with the standard DNS UDP packet size limit, eliminating the need for dynamic memory allocation during normal operation.

### Network Efficiency

- Socket reuse options are set to prevent "Address already in use" errors
- Timeouts are implemented for resolver communication to prevent hanging
- Error conditions are handled gracefully to maintain service availability

## Error Handling

The server implements robust error handling for:

- Socket creation and binding failures
- Malformed DNS packets
- Network communication issues
- Resolver timeouts

All errors are logged with appropriate context information to aid in troubleshooting.

## Security Considerations

While this is a basic DNS forwarding implementation, several security aspects are considered:

- Buffer size validation to prevent overflow attacks
- Proper checking of DNS packet formats
- Reasonable limits on processing multiple questions

Future security enhancements could include:

- DNSSEC validation
- Query rate limiting
- Access control lists

## Extensibility

The server is designed to be extensible for future enhancements:

- Clear separation of packet processing and network handling
- Modular functions for domain name handling
- Well-defined interfaces between components

## Conclusion

The DNS Forwarding Server provides a lightweight yet capable DNS forwarding solution with support for both standard and multi-question DNS queries. Its architecture balances simplicity with functionality, making it suitable for integration into larger systems or use as a standalone DNS forwarding component.