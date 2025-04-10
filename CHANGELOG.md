## [0.2.0] - 2025-04-10

### Added
- DNS header structure implementation compliant with RFC 1035
- Response generation with proper header field values:
  - Query ID (1234)
  - Query/Response bit set to 1 (response)
  - All flag fields set to expected test values
- Network/host byte order conversion functions for DNS headers
- Debug logging for received queries and sent responses

### Changed
- Replaced simple echo response with structured DNS response packet
- Updated packet handling to parse and process DNS-specific data
- Modified buffer handling to work with binary DNS message format

### Technical Notes
- This implementation focuses only on the header section (12 bytes)
- Future work will add support for question, answer, authority, and additional sections
- The server currently uses hardcoded values required by the test suite rather than dynamically responding to query content