# Changelog

All notable changes to the DNS Forwarding Server project will be documented in this file.

## [1.0.0] - 2025-04-13

### Added
- Initial release of DNS forwarding server
- Support for forwarding single-question DNS queries to specified resolver
- Support for handling and responding to multi-question DNS queries
- Domain name compression handling in DNS packets
- Command-line argument parsing for resolver configuration
- Basic error handling and logging

### Technical
- Implemented DNS header parsing and construction
- Added functions for domain name extraction and processing
- Created handlers for both single and multiple question DNS queries
- Implemented socket management for DNS communication
- Added proper byte-order handling for cross-platform compatibility

### Other
- Created project documentation (README, DESIGN, CONTRIBUTING)
- Added build system with Makefile
- Implemented CI/CD workflow
- Established project structure