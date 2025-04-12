#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <arpa/inet.h>

// DNS header structure as per RFC 1035
struct dns_header {
    uint16_t id;      // Identification number
    
    // Flags packed in a single 16-bit value
    uint8_t rd:1;     // Recursion Desired
    uint8_t tc:1;     // Truncated Message
    uint8_t aa:1;     // Authoritative Answer
    uint8_t opcode:4; // Operation code
    uint8_t qr:1;     // Query/Response Indicator
    
    uint8_t rcode:4;  // Response code
    uint8_t z:3;      // Reserved for future use
    uint8_t ra:1;     // Recursion Available
    
    uint16_t qdcount; // Number of question entries
    uint16_t ancount; // Number of answer entries
    uint16_t nscount; // Number of authority entries
    uint16_t arcount; // Number of resource entries
};

// Structure to hold a parsed question
struct dns_question {
    unsigned char *name;      // Pointer to the domain name in buffer
    size_t name_len;          // Length of the name field including terminator
    uint16_t type;            // Question type
    uint16_t class;           // Question class
    unsigned char *name_uncompressed; // Uncompressed version of name
    size_t name_uncompressed_len;     // Length of uncompressed name
};

// Function to convert DNS header to network byte order (big-endian)
void dns_header_to_network(struct dns_header *header) {
    header->id = htons(header->id);
    header->qdcount = htons(header->qdcount);
    header->ancount = htons(header->ancount);
    header->nscount = htons(header->nscount);
    header->arcount = htons(header->arcount);
}

// Function to convert DNS header from network byte order (big-endian) to host byte order
void dns_header_from_network(struct dns_header *header) {
    header->id = ntohs(header->id);
    header->qdcount = ntohs(header->qdcount);
    header->ancount = ntohs(header->ancount);
    header->nscount = ntohs(header->nscount);
    header->arcount = ntohs(header->arcount);
}

// Function to uncompress a compressed domain name
// Returns a newly allocated buffer with uncompressed name
unsigned char *uncompress_name(const unsigned char *buffer, const unsigned char *name, size_t *uncompressed_len) {
    const unsigned char *packet_start = buffer;
    const unsigned char *current = name;
    
    // Estimate maximum size (worst case: completely uncompressed)
    unsigned char *result = (unsigned char*)malloc(256);
    unsigned char *result_ptr = result;
    size_t total_len = 0;
    
    while (*current) {
        if ((*current & 0xC0) == 0xC0) {
            // This is a pointer (compressed name)
            uint16_t offset = ((*current & 0x3F) << 8) | *(current + 1);
            current = packet_start + offset;
        } else {
            // This is a regular label
            uint8_t label_len = *current;
            
            // Copy length byte
            *result_ptr++ = label_len;
            total_len++;
            
            // Copy the label
            memcpy(result_ptr, current + 1, label_len);
            result_ptr += label_len;
            total_len += label_len;
            
            // Move to next label
            current += label_len + 1;
        }
    }
    
    // Add terminator
    *result_ptr = 0;
    total_len++;
    
    *uncompressed_len = total_len;
    return result;
}

// Function to parse a question from a DNS packet
// Returns pointer to the byte after the question
unsigned char *parse_question(const unsigned char *buffer, 
                              const unsigned char *question_start, 
                              struct dns_question *question) {
    unsigned char *current = (unsigned char*)question_start;
    question->name = current;
    
    // Find the end of the domain name
    while (*current) {
        if ((*current & 0xC0) == 0xC0) {
            // This is a compressed name pointer (2 bytes)
            current += 2;
            break;
        } else {
            // This is a regular label
            uint8_t label_len = *current;
            current += label_len + 1;
        }
    }
    
    // If we didn't end with a pointer, skip the terminating null byte
    if (*(current - 1) != 0 && (*(current - 2) & 0xC0) != 0xC0) {
        current++;
    }
    
    // Calculate name field length
    question->name_len = current - question_start;
    
    // Extract QTYPE and QCLASS
    memcpy(&question->type, current, sizeof(question->type));
    current += sizeof(question->type);
    memcpy(&question->class, current, sizeof(question->class));
    current += sizeof(question->class);
    
    // Convert from network byte order
    question->type = ntohs(question->type);
    question->class = ntohs(question->class);
    
    // Uncompress the name
    question->name_uncompressed = uncompress_name(buffer, question_start, &question->name_uncompressed_len);
    
    return current;
}

int main() {
    // Disable output buffering
    setbuf(stdout, NULL);
    setbuf(stderr, NULL);

    // You can use print statements as follows for debugging, they'll be visible when running tests.
    printf("Logs from your program will appear here!\n");

    int udpSocket, client_addr_len;
    struct sockaddr_in clientAddress;
    
    udpSocket = socket(AF_INET, SOCK_DGRAM, 0);
    if (udpSocket == -1) {
        printf("Socket creation failed: %s...\n", strerror(errno));
        return 1;
    }
    
    // Since the tester restarts your program quite often, setting REUSE_PORT
    // ensures that we don't run into 'Address already in use' errors
    int reuse = 1;
    if (setsockopt(udpSocket, SOL_SOCKET, SO_REUSEPORT, &reuse, sizeof(reuse)) < 0) {
        printf("SO_REUSEPORT failed: %s \n", strerror(errno));
        return 1;
    }
    
    struct sockaddr_in serv_addr = { 
        .sin_family = AF_INET,
        .sin_port = htons(2053),
        .sin_addr = { htonl(INADDR_ANY) },
    };
    
    if (bind(udpSocket, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) != 0) {
        printf("Bind failed: %s \n", strerror(errno));
        return 1;
    }

    int bytesRead;
    unsigned char buffer[512];
    socklen_t clientAddrLen = sizeof(clientAddress);
    
    while (1) {
        // Receive data
        bytesRead = recvfrom(udpSocket, buffer, sizeof(buffer), 0, (struct sockaddr*)&clientAddress, &clientAddrLen);
        if (bytesRead == -1) {
            perror("Error receiving data");
            break;
        }
        
        printf("Received %d bytes\n", bytesRead);
        
        // Check if received data is a DNS query (at least 12 bytes for the header)
        if (bytesRead < 12) {
            printf("Received packet too small to be a DNS query\n");
            continue;
        }
        
        // Parse the DNS header from the received data
        struct dns_header query_header;
        memcpy(&query_header, buffer, sizeof(struct dns_header));
        
        // Convert header from network byte order
        dns_header_from_network(&query_header);
        
        printf("Received DNS query with ID: %d, OPCODE: %d, RD: %d, QDCOUNT: %d\n", 
               query_header.id, query_header.opcode, query_header.rd, query_header.qdcount);
        
        // Create response header with values from the request
        struct dns_header response_header = {0};
        response_header.id = query_header.id;  // Use the ID from the request
        response_header.qr = 1;                // This is a response
        response_header.opcode = query_header.opcode; // Use OPCODE from request
        response_header.aa = 0;                // Not authoritative
        response_header.tc = 0;                // Not truncated
        response_header.rd = query_header.rd;  // Use RD from request
        response_header.ra = 0;                // Recursion not available
        response_header.z = 0;                 // Reserved bits
        
        // Set RCODE based on OPCODE
        if (query_header.opcode == 0) {
            response_header.rcode = 0;         // No error for standard query
        } else {
            response_header.rcode = 4;         // Not implemented for other opcodes
        }
        
        response_header.qdcount = query_header.qdcount; // Same number of questions
        response_header.ancount = query_header.qdcount; // Same number of answers as questions
        response_header.nscount = 0;
        response_header.arcount = 0;
        
        // Convert response header to network byte order
        struct dns_header network_response_header = response_header;
        dns_header_to_network(&network_response_header);
        
        // Parse all questions
        struct dns_question *questions = (struct dns_question*)malloc(query_header.qdcount * sizeof(struct dns_question));
        
        unsigned char *current_ptr = buffer + sizeof(struct dns_header);
        for (int i = 0; i < query_header.qdcount; i++) {
            printf("Parsing question %d\n", i+1);
            current_ptr = parse_question(buffer, current_ptr, &questions[i]);
            printf("Question %d: TYPE=%d, CLASS=%d\n", i+1, questions[i].type, questions[i].class);
        }
        
        // Calculate response size
        size_t response_size = sizeof(network_response_header);
        
        // Add space for questions (uncompressed)
        for (int i = 0; i < query_header.qdcount; i++) {
            response_size += questions[i].name_uncompressed_len + sizeof(uint16_t) * 2; // name + type + class
        }
        
        // Add space for answers
        for (int i = 0; i < query_header.qdcount; i++) {
            response_size += questions[i].name_uncompressed_len + // name
                          sizeof(uint16_t) + // type
                          sizeof(uint16_t) + // class
                          sizeof(uint32_t) + // ttl
                          sizeof(uint16_t) + // data length
                          sizeof(uint32_t);  // ip address (4 bytes)
        }
        
        // Allocate response buffer
        unsigned char *response = (unsigned char *)malloc(response_size);
        if (!response) {
            printf("Failed to allocate memory for response\n");
            continue;
        }
        
        // Copy header to response
        memcpy(response, &network_response_header, sizeof(network_response_header));
        size_t offset = sizeof(network_response_header);
        
        // Copy uncompressed questions to response
        for (int i = 0; i < query_header.qdcount; i++) {
            // Copy uncompressed name
            memcpy(response + offset, questions[i].name_uncompressed, questions[i].name_uncompressed_len);
            offset += questions[i].name_uncompressed_len;
            
            // Copy type (A record = 1)
            uint16_t type = htons(1);
            memcpy(response + offset, &type, sizeof(type));
            offset += sizeof(type);
            
            // Copy class (IN = 1)
            uint16_t class = htons(1);
            memcpy(response + offset, &class, sizeof(class));
            offset += sizeof(class);
        }
        
        // Add answers
        for (int i = 0; i < query_header.qdcount; i++) {
            // Copy uncompressed name for answer (same as question)
            memcpy(response + offset, questions[i].name_uncompressed, questions[i].name_uncompressed_len);
            offset += questions[i].name_uncompressed_len;
            
            // Type = 1 (A record), in network byte order
            uint16_t answer_type = htons(1);
            memcpy(response + offset, &answer_type, sizeof(answer_type));
            offset += sizeof(answer_type);
            
            // Class = 1 (IN class), in network byte order
            uint16_t answer_class = htons(1);
            memcpy(response + offset, &answer_class, sizeof(answer_class));
            offset += sizeof(answer_class);
            
            // TTL = 60 seconds, in network byte order
            uint32_t answer_ttl = htonl(60);
            memcpy(response + offset, &answer_ttl, sizeof(answer_ttl));
            offset += sizeof(answer_ttl);
            
            // Data length = 4 bytes (for IPv4 address), in network byte order
            uint16_t answer_length = htons(4);
            memcpy(response + offset, &answer_length, sizeof(answer_length));
            offset += sizeof(answer_length);
            
            // IP address (using 8.8.8.8 + question number for uniqueness)
            uint32_t answer_ip = inet_addr("8.8.8.8") + i;
            memcpy(response + offset, &answer_ip, sizeof(answer_ip));
            offset += sizeof(answer_ip);
        }
        
        // Send response
        if (sendto(udpSocket, response, response_size, 0, 
                  (struct sockaddr*)&clientAddress, sizeof(clientAddress)) == -1) {
            perror("Failed to send response");
        } else {
            printf("Sent DNS response with ID: %d, %d questions and %d answers\n", 
                   response_header.id, response_header.qdcount, response_header.ancount);
        }
        
        // Free memory
        for (int i = 0; i < query_header.qdcount; i++) {
            free(questions[i].name_uncompressed);
        }
        free(questions);
        free(response);
    }
    
    close(udpSocket);

    return 0;
}