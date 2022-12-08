//
// Created by Sidney Goldinger on 11/30/22.
//

#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <string_view>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <string.h>
#include <string>
#include <arpa/inet.h>
#include <netinet/udp.h>
#include <netinet/ip.h>
#include <ctime>
#include <unistd.h>

// Set the following port to a unique number:
#define MYPORT 5950
#define NEXT_HOP "10.63.36.2" //temporary constant for testing
#define ROUTER "10.63.36.1"

void router();
void endhost();
int main(int, char**);
int create_cs3516_socket();
int cs3516_recv(int , void *, int);
int cs3516_send(int, void *, int, unsigned long);

/**
 * random alpha-numeric string generation
 * @param len length of the string
 * @return a random alpha-numeric string of len characters
 */
std::string random_string(const int len) {
    srand(time(NULL));

    static const char alphanum[] =
            "0123456789"
            "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
            "abcdefghijklmnopqrstuvwxyz";
    std::string tmp_s;
    tmp_s.reserve(len);

    for (int i = 0; i < len; ++i) {
        tmp_s += alphanum[rand() % (sizeof(alphanum) - 1)];
    }

    return tmp_s;
}

void router() {
    printf("router\n");
    printf("Attempting to create a socket...\n");
    int sockfd = create_cs3516_socket();

    //receive data length first somehow. Look at project 1?

    //receive data
    int bufferSize = 1000;
    u_char buffer[bufferSize];
    u_int32_t length;
    memset(buffer, 0, bufferSize);
    //always try to recv()
    while(1) {
        // recieving IP and UDP headers n shit
        printf("Attempting to recv() length...\n");
        memset(buffer, 0, bufferSize);
        cs3516_recv(sockfd, &length, bufferSize);
        printf("%d\n", length);
        printf("Attempting to recv() data...\n");
        cs3516_recv(sockfd, buffer, bufferSize);
        printf("%s\n", buffer);
        memset(buffer, 0, bufferSize);
        cs3516_recv(sockfd, buffer, bufferSize);
        struct ip* ip_header = ((struct ip*) buffer);
        printf("%s\n", buffer);
        printf("%u\n", ip_header->ip_ttl);

        // TODO below here in this infinite loop untested but builds
        // decrementing TTL
        ip_header->ip_ttl--;

        // forward to hardcoded router
        struct in_addr inp;
        inet_aton(NEXT_HOP, &inp);
        printf("Attempting to send() to next hop...\n");
        cs3516_send(sockfd, buffer, bufferSize, inp.s_addr);
        // TODO above here in this infinite loop untested but builds
    }

}

// struct udphdr {
//        u16 src;
//        u16 dest;
//        u16 len;
//        u16 checksum;
//     } __packed;

// struct ip {
// #if BYTE_ORDER == LITTLE_ENDIAN 
//     u_char  ip_hl:4,        /* header length */
//         ip_v:4;         /* version */
// #endif
// #if BYTE_ORDER == BIG_ENDIAN 
//     u_char  ip_v:4,         /* version */
//         ip_hl:4;        /* header length */
// #endif
//     u_char  ip_tos;         /* type of service */
//     short   ip_len;         /* total length */
//     u_short ip_id;          /* identification */
//     short   ip_off;         /* fragment offset field */
// #define IP_DF 0x4000            /* dont fragment flag */
// #define IP_MF 0x2000            /* more fragments flag */
//     u_char  ip_ttl;         /* time to live */
//     u_char  ip_p;           /* protocol */
//     u_short ip_sum;         /* checksum */
//     struct  in_addr ip_src,ip_dst;  /* source and dest address */
// };

void endhost() {
    // end hosts should be able to send data across the network 
    // and receive data from the router it is directly connected to

    // reading send_config


    // reading send_body

    // socket
    printf("host\n");
    printf("Attempting to create a socket...\n");
    int sockfd = create_cs3516_socket();
    struct in_addr inp;
    inet_aton(ROUTER, &inp);

    //form data of some kind to send
    int bufferSize = 64;
    char buffer[bufferSize];
    int len = 20;
    std::string random_string_std = random_string(len);
    char* rand_str = const_cast<char*>(random_string_std.c_str()); // converting std::string to char *
    //std::cout << rand_str << "\n"; // testing the random string generator (it works tho)
    strcpy(buffer, rand_str);

    // strcat(udpHeader, buffer);
    // strcat(ipHeader, udpHeader);

    //construct headers
    // IP header
    int ipHeaderBufferSize = sizeof(struct ip);
    u_char ipHeaderBuffer[ipHeaderBufferSize];
    struct ip ip_hdr;
    ip_hdr.ip_ttl = 50;
    struct ip* ip_header = &ip_hdr;
    printf("before\n");
    //ip_header->ip_ttl = 50; //test constant
    printf("after\n");
    //inet_ntoa((struct in_addr)ip_header->ip_src
    //ip_header.ip_src = inet_aton();
    memcpy(ipHeaderBuffer, ip_header, sizeof(struct ip));
    // //append headers to it somehow
    u_char concatBuffer[bufferSize + ipHeaderBufferSize];
    printf("%s\n", buffer);
    //strcat(ipHeaderBuffer, buffer);
    memcpy(concatBuffer, ipHeaderBuffer, ipHeaderBufferSize);
    memcpy(concatBuffer + ipHeaderBufferSize, buffer, bufferSize);
    //printf("%s\n", ipHeaderBuffer);

    // UDP header
    int udpHeaderBufferSize = sizeof(struct udphdr);
    u_char udpHeaderBuffer[udpHeaderBufferSize];
    struct udphdr udp_hdr;
             // udp_hdr.udp_ttl = 50; // TODO does this work?
    struct udphdr* udp_header = &udp_hdr;


    // sending IP things
    //send length of data first
    printf("Attempting to send() length...\n");
    u_int32_t length = len;
    u_int32_t* lengthP = &length;

    if(cs3516_send(sockfd, lengthP, sizeof(u_int32_t), inp.s_addr) == -1) {
        perror("error with sending the length");
        exit(1);
    }

    //send data (with headers) over the network
    printf("Attempting to send() data...\n");
    //destination IP address
    if(cs3516_send(sockfd, buffer, bufferSize, inp.s_addr) == -1) {
        perror("error with sending the data");
        exit(1);
    }

    if(cs3516_send(sockfd, ipHeaderBuffer, ipHeaderBufferSize, inp.s_addr) == -1) {
        perror("error with sending the data");
        exit(1);
    }

    // sending UDP things

}

int main (int argc, char **argv) {
    // test if end-host or router
    std::string arg1 = argv[1];

    if (arg1 == "r") { router(); }
    else if (arg1 == "h") { endhost(); }
    else { printf("please input r or h.\n"); return 0; }

    return 0;
}


int create_cs3516_socket() {
    int sock;
    struct sockaddr_in server;
    
    sock = socket(AF_INET, SOCK_DGRAM, 0);

    if (sock < 0) herror("Error creating CS3516 socket");

    bzero(&server, sizeof(server));
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons(MYPORT);
    if (bind(sock, (struct sockaddr *) &server, sizeof(server) ) < 0) 
        herror("Unable to bind CS3516 socket");

    // Socket is now bound:
    return sock;
}

int cs3516_recv(int sock, void *buffer, int buff_size) {
    struct sockaddr_in from;
    int fromlen, n;
    fromlen = sizeof(struct sockaddr_in);
    n = recvfrom(sock, buffer, buff_size, 0,
                 (struct sockaddr *) &from, (socklen_t*)&fromlen);

    return n;
}

int cs3516_send(int sock, void *buffer, int buff_size, unsigned long nextIP) {
    struct sockaddr_in to;
    int tolen, n;

    tolen = sizeof(struct sockaddr_in);

    // Okay, we must populate the to structure. 
    bzero(&to, sizeof(to));
    to.sin_family = AF_INET;
    to.sin_port = htons(MYPORT);
    to.sin_addr.s_addr = nextIP;

    // We can now send to this destination:
    n = sendto(sock, buffer, buff_size, 0,
               (struct sockaddr *)&to, tolen);

    return n;
}
