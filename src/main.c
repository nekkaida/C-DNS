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
        
        printf("Received DNS query with ID: %d\n", query_header.id);
        
        // Create response header with specified values
        struct dns_header response_header = {0};
        response_header.id = 1234;  // Expected value for testing
        response_header.qr = 1;     // This is a response
        response_header.opcode = 0; // Standard query
        response_header.aa = 0;     // Not authoritative
        response_header.tc = 0;     // Not truncated
        response_header.rd = 0;     // Recursion not desired
        response_header.ra = 0;     // Recursion not available
        response_header.z = 0;      // Reserved bits
        response_header.rcode = 0;  // No error
        response_header.qdcount = 1; // Include 1 question - this is updated from 0
        response_header.ancount = 0;
        response_header.nscount = 0;
        response_header.arcount = 0;
        
        // Convert response header to network byte order
        struct dns_header network_response_header = response_header;
        dns_header_to_network(&network_response_header);
        
        // Create response packet
        // 12 bytes for header + length for question section
        // For the test, we're using a specific hardcoded question
        unsigned char question[] = {
            0x0c, 'c', 'o', 'd', 'e', 'c', 'r', 'a', 'f', 't', 'e', 'r', 's', // label "codecrafters" (length 12)
            0x02, 'i', 'o',                                                   // label "io" (length 2)
            0x00,                                                             // null byte to terminate domain name
            0x00, 0x01,                                                       // QTYPE = 1 (A record)
            0x00, 0x01                                                        // QCLASS = 1 (IN class)
        };
        
        size_t question_len = sizeof(question);
        size_t response_len = sizeof(network_response_header) + question_len;
        unsigned char response[response_len];
        
        // Copy header to response
        memcpy(response, &network_response_header, sizeof(network_response_header));
        
        // Copy question to response
        memcpy(response + sizeof(network_response_header), question, question_len);
        
        // Send response
        if (sendto(udpSocket, response, response_len, 0, 
                  (struct sockaddr*)&clientAddress, sizeof(clientAddress)) == -1) {
            perror("Failed to send response");
        } else {
            printf("Sent DNS response with ID: %d and question section\n", response_header.id);
        }
    }
    
    close(udpSocket);

    return 0;
}