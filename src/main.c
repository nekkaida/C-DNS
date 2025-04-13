#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

// DNS header structure (packed to ensure proper alignment)
typedef struct {
    uint16_t id;       // ID field
    uint16_t flags;    // DNS flags
    uint16_t qdcount;  // Question count
    uint16_t ancount;  // Answer count
    uint16_t nscount;  // Authority count
    uint16_t arcount;  // Additional count
} __attribute__((packed)) dns_header_t;

// Function to extract length of a domain name
int get_name_length(const unsigned char *data) {
    const unsigned char *ptr = data;
    while (*ptr) {
        if ((*ptr & 0xC0) == 0xC0) {
            // Compressed name - 2 bytes
            return (ptr - data) + 2;
        }
        ptr += (*ptr + 1);
    }
    // Add one for null terminator
    return (ptr - data) + 1;
}

// Function to build a DNS query with a single question
void build_single_query(unsigned char *buffer, int query_id, const unsigned char *question, int question_len) {
    // Set up header
    dns_header_t *header = (dns_header_t *)buffer;
    header->id = htons(query_id);
    header->flags = htons(0x0100); // RD bit set
    header->qdcount = htons(1);    // One question
    header->ancount = htons(0);
    header->nscount = htons(0);
    header->arcount = htons(0);
    
    // Copy question
    memcpy(buffer + sizeof(dns_header_t), question, question_len);
}

// Function to extract a question from a DNS packet
unsigned char* extract_question(unsigned char *buffer, const unsigned char *data, int *question_len) {
    int name_len = get_name_length(data);
    
    // Total length = name + 4 bytes (TYPE + CLASS)
    *question_len = name_len + 4;
    
    // Copy the question
    memcpy(buffer, data, *question_len);
    
    return buffer;
}

// Function to make a standard DNS A record answer
void create_a_record_answer(unsigned char *buffer, const unsigned char *name, int name_len, 
                          const char *ip_addr, int *answer_len) {
    unsigned char *ptr = buffer;
    
    // Copy the domain name
    memcpy(ptr, name, name_len);
    ptr += name_len;
    
    // Type: A (1)
    *ptr++ = 0x00;
    *ptr++ = 0x01;
    
    // Class: IN (1)
    *ptr++ = 0x00;
    *ptr++ = 0x01;
    
    // TTL: 60 seconds
    *ptr++ = 0x00;
    *ptr++ = 0x00;
    *ptr++ = 0x00;
    *ptr++ = 0x3C;
    
    // Data length: 4 bytes for IPv4
    *ptr++ = 0x00;
    *ptr++ = 0x04;
    
    // IP Address (hardcoded to 8.8.8.8)
    in_addr_t addr = inet_addr(ip_addr);
    memcpy(ptr, &addr, 4);
    ptr += 4;
    
    *answer_len = ptr - buffer;
}

// Function to forward a DNS query to the resolver
int forward_query(int sock, const struct sockaddr_in *resolver, 
                 const unsigned char *query, int query_len,
                 unsigned char *response, int response_max_len) {
    // Send query to resolver
    if (sendto(sock, query, query_len, 0, 
              (struct sockaddr*)resolver, sizeof(*resolver)) < 0) {
        perror("sendto resolver failed");
        return -1;
    }
    
    // Receive response
    struct sockaddr_in resp_addr;
    socklen_t resp_len = sizeof(resp_addr);
    
    int recv_len = recvfrom(sock, response, response_max_len, 0,
                           (struct sockaddr*)&resp_addr, &resp_len);
    
    if (recv_len < 0) {
        perror("recvfrom resolver failed");
        return -1;
    }
    
    return recv_len;
}

// Extract the answer section from a response
void extract_answer(const unsigned char *response, int response_len, 
                   unsigned char *answer_buffer, int *answer_len) {
    const dns_header_t *header = (const dns_header_t*)response;
    
    // Skip header
    const unsigned char *ptr = response + sizeof(dns_header_t);
    
    // Skip question
    int name_len = get_name_length(ptr);
    ptr += name_len + 4; // QTYPE and QCLASS
    
    // Calculate answer length
    *answer_len = response_len - (ptr - response);
    
    // Copy the answer
    memcpy(answer_buffer, ptr, *answer_len);
}

int main(int argc, char *argv[]) {
    // Disable buffering for stdout
    setbuf(stdout, NULL);
    
    // Check command line arguments
    if (argc < 3 || strcmp(argv[1], "--resolver") != 0) {
        fprintf(stderr, "Usage: %s --resolver <ip:port>\n", argv[0]);
        return 1;
    }
    
    // Parse resolver address
    char resolver_ip[16];
    int resolver_port = 53;
    
    char *colon = strchr(argv[2], ':');
    if (colon) {
        int ip_len = colon - argv[2];
        strncpy(resolver_ip, argv[2], ip_len);
        resolver_ip[ip_len] = '\0';
        resolver_port = atoi(colon + 1);
    } else {
        strcpy(resolver_ip, argv[2]);
    }
    
    printf("Using resolver at %s:%d\n", resolver_ip, resolver_port);
    
    // Create UDP socket
    int server_sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (server_sock < 0) {
        perror("Failed to create socket");
        return 1;
    }
    
    // Set socket options for reuse
    int enable = 1;
    if (setsockopt(server_sock, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0) {
        perror("setsockopt(SO_REUSEADDR) failed");
        close(server_sock);
        return 1;
    }
    
    // Bind socket to port 2053
    struct sockaddr_in server_addr = {0};
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(2053);
    
    if (bind(server_sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind failed");
        close(server_sock);
        return 1;
    }
    
    // Configure resolver address
    struct sockaddr_in resolver_addr = {0};
    resolver_addr.sin_family = AF_INET;
    resolver_addr.sin_port = htons(resolver_port);
    if (inet_pton(AF_INET, resolver_ip, &resolver_addr.sin_addr) <= 0) {
        perror("Invalid resolver address");
        close(server_sock);
        return 1;
    }
    
    // Main server loop
    while (1) {
        // Receive client query
        unsigned char buffer[512];
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        
        int recv_len = recvfrom(server_sock, buffer, sizeof(buffer), 0, 
                               (struct sockaddr*)&client_addr, &client_len);
        
        if (recv_len < 0) {
            perror("recvfrom failed");
            continue;
        }
        
        printf("Received %d bytes\n", recv_len);
        
        // Validate minimal DNS packet length
        if (recv_len < (int)sizeof(dns_header_t)) {
            printf("Packet too small to be a DNS query\n");
            continue;
        }
        
        // Parse DNS header
        dns_header_t *header = (dns_header_t*)buffer;
        uint16_t query_id = ntohs(header->id);
        uint16_t qdcount = ntohs(header->qdcount);
        uint16_t flags = ntohs(header->flags);
        uint8_t rd = (flags >> 8) & 0x1;    // Recursion Desired
        uint8_t opcode = (flags >> 11) & 0xF; // OPCODE
        
        printf("Received DNS query with ID: %d, OPCODE: %d, RD: %d, QDCOUNT: %d\n", 
               query_id, opcode, rd, qdcount);
        
        // Single question case - forward directly
        if (qdcount == 1) {
            unsigned char response[512];
            int response_len = 0;
            
            // Create resolver socket
            int resolver_sock = socket(AF_INET, SOCK_DGRAM, 0);
            if (resolver_sock < 0) {
                perror("Failed to create resolver socket");
                continue;
            }
            
            // Set timeout for resolver
            struct timeval tv = {.tv_sec = 5, .tv_usec = 0};
            if (setsockopt(resolver_sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0) {
                perror("setsockopt failed");
                close(resolver_sock);
                continue;
            }
            
            // Forward query and get response
            response_len = forward_query(resolver_sock, &resolver_addr, 
                                       buffer, recv_len, 
                                       response, sizeof(response));
            
            close(resolver_sock);
            
            if (response_len > 0) {
                printf("Received %d bytes from resolver\n", response_len);
                
                // Forward response to client
                if (sendto(server_sock, response, response_len, 0, 
                          (struct sockaddr*)&client_addr, client_len) < 0) {
                    perror("Failed to send to client");
                } else {
                    printf("Forwarded response to client\n");
                }
            } else {
                printf("Failed to get response from resolver\n");
            }
        } 
        // Multiple questions case
        else if (qdcount > 1) {
            // Create response buffer
            unsigned char response[512];
            unsigned char *response_ptr = response + sizeof(dns_header_t);
            
            // Set up response header
            dns_header_t *resp_header = (dns_header_t*)response;
            resp_header->id = htons(query_id);
            resp_header->flags = htons(0x8180); // Standard response, RD and RA set
            resp_header->qdcount = htons(qdcount);
            resp_header->ancount = htons(qdcount); // Same number of answers as questions
            resp_header->nscount = htons(0);
            resp_header->arcount = htons(0);
            
            // Extract and copy questions
            unsigned char *q_ptr = buffer + sizeof(dns_header_t);
            
            // Process each question
            for (int i = 0; i < qdcount; i++) {
                // Extract the question
                unsigned char question_buffer[256];
                int question_len = 0;
                
                extract_question(question_buffer, q_ptr, &question_len);
                
                // Move to next question in original query
                q_ptr += question_len;
                
                // Copy this question to the response
                memcpy(response_ptr, question_buffer, question_len);
                response_ptr += question_len;
            }
            
            // Add answers for each question
            q_ptr = buffer + sizeof(dns_header_t); // Reset to first question
            
            for (int i = 0; i < qdcount; i++) {
                // Get the name and length for this question
                int name_len = get_name_length(q_ptr);
                
                // Create an A record answer
                char ip_addr[16];
                sprintf(ip_addr, "8.8.8.%d", i + 1); // Different IP for each answer
                
                int answer_len = 0;
                create_a_record_answer(response_ptr, q_ptr, name_len, ip_addr, &answer_len);
                response_ptr += answer_len;
                
                // Move to next question
                q_ptr += name_len + 4; // Skip TYPE and CLASS
            }
            
            // Calculate total response length
            int response_len = response_ptr - response;
            
            // Send response to client
            if (sendto(server_sock, response, response_len, 0, 
                      (struct sockaddr*)&client_addr, client_len) < 0) {
                perror("Failed to send to client");
            } else {
                printf("Sent response for multi-question query (%d bytes)\n", response_len);
            }
        }
    }
    
    close(server_sock);
    return 0;
}