# DNS Forwarding Server

A lightweight, high-performance DNS forwarding server implemented in C that efficiently handles both single and multiple question DNS queries.

## Overview

This DNS forwarding server receives DNS queries, forwards them to an upstream resolver, and returns the responses to the client. It fully supports DNS message compression according to RFC 1035 and can handle multiple questions in a single DNS query.

## Features

- Forwards DNS queries to a configurable upstream resolver
- Handles both single and multiple question DNS queries
- Supports DNS name compression in queries
- Provides proper error handling and logging
- Minimal resource requirements and high performance
- Fully compliant with DNS protocol standards

## Requirements

- C compiler (GCC or Clang recommended)
- POSIX-compliant operating system
- Make build system

## Installation

Clone the repository and build the server:

```bash
git clone https://github.com/yourusername/dns-forwarding-server.git
cd dns-forwarding-server
make
```

## Usage

Run the DNS server with a specified upstream resolver:

```bash
./dns-server --resolver <ip:port>
```

Example:
```bash
./dns-server --resolver 8.8.8.8:53
```

The server listens on UDP port 2053 for incoming DNS requests.

## Technical Details

### Single-Question Queries

For queries with a single question, the server forwards the entire query directly to the upstream resolver and returns the received response to the client.

### Multi-Question Queries

For queries with multiple questions, the server:
1. Parses all questions from the incoming query
2. Constructs a response containing all questions and appropriate answers
3. Sends the combined response back to the client

### DNS Name Compression

The server fully supports DNS name compression, including:
- Decompressing compressed domain names in queries
- Handling pointer references to previous occurrences of domain names
- Proper extraction of domain names regardless of compression

## Development

This project is designed to be extensible. Future enhancements might include:

- Support for additional DNS record types
- DNS query caching
- DNSSEC validation
- Configuration file support
- IPv6 support

## Documentation

- [DESIGN.md](DESIGN.md) - Detailed design and architecture documentation
- [CONTRIBUTING.md](CONTRIBUTING.md) - Guidelines for contributing
- [CHANGELOG.md](CHANGELOG.md) - Project history and version information

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.