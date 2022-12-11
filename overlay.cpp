//
// Created by Sidney Goldinger on 11/30/22.
//

#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <fstream>
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
#include <sys/time.h>
#include <queue>
#include <map>
using namespace std;

// Set the following port to a unique number:
#define MYPORT 5950
#define NEXT_HOP "10.63.36.2" //temporary constant for testing
#define ROUTER "10.63.36.1"
#define SOURCE "1.2.3.1"
#define DEST "4.5.6.1"
#define ROUTER_IP "10.63.36.1"

void router();
void endhost();
int main(int, char**);
int create_cs3516_socket();
int cs3516_recv(int , void *, int);
int cs3516_send(int, void *, int, unsigned long);

struct packet {
    int bufferSize;
    struct timeval deadLine;
    struct in_addr next_hop;
    u_char* buffer;
};

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
    fd_set read_fd;
    printf("router\n");
    printf("Attempting to create a socket...\n");
    int sockfd = create_cs3516_socket();
    std::queue<struct packet> queue;
    std::map<std::string, std::string> map8;
    std::map<std::string, std::string> map16;
    std::map<std::string, std::string> map24;

    FILE* configFile = fopen("config.txt", "r");
    int routerID;
    //find self
        //loop through all lines
        //if starts with 1
            //check last spot and compare with this routers real IP
            //if found, the number in the second spot is this routers ID
    //find what devices this router is directly connected to
        //check all lines that start with 3 or 4
        //if second num == routerID
            //if first num == 4
                //map 4th spot to 5th num
                //(prefix to host ID)
            //if first num == 3
                //map ovleray prefix of second num with second num
    //

    // file things
    ofstream of;
    fstream f;

    //receive data
    int bufferSize = 1000;
    u_char buffer[bufferSize];
    u_int32_t length;
    memset(buffer, 0, bufferSize);
    struct timeval timeout;
    //always try to recv()
    while(1) {
        //printf("in loop\n");
        struct timeval currentTime;
        gettimeofday(&currentTime, NULL);
        long int ms = currentTime.tv_sec * 1000 + currentTime.tv_usec / 1000;
        timeout.tv_sec = 0;
		timeout.tv_usec = 10000;
        FD_ZERO(&read_fd);
        FD_SET(sockfd, &read_fd);
        int r = select(sockfd + 1, &read_fd, NULL, NULL, &timeout);
        if(r == 0) {
            //printf("checking queue\n");
            //timeout, check the queue
            if(queue.size() > 0) {
                //printf("before\n");
                printf("%s\n", queue.front().buffer + sizeof(struct ip) + sizeof(struct udphdr));
                //printf("after\n");
                printf("%d\n", queue.front().bufferSize);
                printf("%s\n", inet_ntoa(queue.front().next_hop));
                if(timercmp(&currentTime, &queue.front().deadLine, >=)){
                    printf("Forwarding!\n");
                    cs3516_send(sockfd, queue.front().buffer, queue.front().bufferSize, queue.front().next_hop.s_addr);
                    queue.pop();
                }
            }
        } else if(r < 0) {
            //error
            printf("error\n");
        } else {
            printf("tyring to recv()\n");
            //ready to recvfrom() something
            struct packet queueEntry;
            //https://stackoverflow.com/questions/19555121/how-to-get-current-timestamp-in-milliseconds-since-1970-just-the-way-java-gets

            printf("current time (ms): %ld\n", ms);
            // recieving IP and UDP headers n shit
            printf("Attempting to recv() length...\n");
            memset(buffer, 0, bufferSize);
            cs3516_recv(sockfd, &length, bufferSize);
            printf("%d\n", length);
            queueEntry.bufferSize = length;
            printf("Attempting to recv() data...\n");
            // cs3516_recv(sockfd, buffer, bufferSize);
            // printf("%s\n", buffer);
            // memset(buffer, 0, bufferSize);
            // cs3516_recv(sockfd, buffer, bufferSize);
            memset(buffer, 0, bufferSize);
            cs3516_recv(sockfd, buffer, bufferSize);
            struct ip *ip_header = ((struct ip *) buffer);
            printf("%s\n", buffer);
            printf("%u\n", ip_header->ip_ttl);
            printf("%s\n", inet_ntoa(ip_header->ip_src));
            printf("%s\n", inet_ntoa(ip_header->ip_dst));
            //print data
            printf("%s\n", buffer + sizeof(struct ip) + sizeof(struct udphdr));
            //printf("%s\n", buffer+sizeof(struct udphdr)+sizeof(struct iphdr));
            u_char* bufferCopy = (u_char*)malloc(bufferSize); 
            memcpy(bufferCopy, buffer, bufferSize);
            //printf("before\n");
            queueEntry.buffer = (u_char*)malloc(bufferSize);
            memcpy(queueEntry.buffer, bufferCopy, bufferSize);
            //printf("after\n");
            printf("%s\n", queueEntry.buffer + sizeof(struct ip) + sizeof(struct udphdr));
            struct timeval delay;
            //delay will differ based on where the packet is going
            //just more info we need to parse from config file
            delay.tv_sec = 1; 
            timeradd(&delay, &currentTime, &queueEntry.deadLine);
            
            if (ip_header->ip_ttl > 0) {
                if (strcmp(inet_ntoa(ip_header->ip_dst), "4.5.6.1") == 0) {
                    //send to real address of "4.5.6.1"
                    struct in_addr next;
                    inet_aton("10.63.36.3", &next);
                    printf("Enqueuing\n");
                    queueEntry.next_hop = next;
                    //cs3516_send(sockfd, buffer, bufferSize, next.s_addr);
                }
                else {
                    //else send to the other option
                    printf("Enqueuing for other dest\n");
                    struct in_addr next;
                    inet_aton("10.63.36.4", &next);
                    queueEntry.next_hop = next;
                    //cs3516_send(sockfd, buffer, bufferSize, next.s_addr);
                }
                queue.push(queueEntry);
            }
            else { // if it's zero, log
                of.open("router_log.txt", ios::app);
                if (!of) { cout << "No such file found"; }
                else {
                    of << " packet dropped because ttl = 0. \n";
                    of.close();

                }
            }
        }
        
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
    fd_set read_fd;
    // end hosts should be able to send data across the network 
    // and receive data from the router it is directly connected to

    // reading send_config

    /*
    std::fstream file;
    std::string word, t, q, filename;
    filename = "file.txt";
    file.open(filename.c_str());


    int i = 0;
    while (file >> word) {

        i = i + 1;
    }
     */

    // reading send_body

    // socket
    printf("host\n");
    printf("Attempting to create a socket...\n");
    int sockfd = create_cs3516_socket();
    struct in_addr inp;
    struct in_addr sourceIP;
    struct in_addr destIP;
    std::queue<struct packet> queue;
    inet_aton(ROUTER, &inp);
    inet_aton(SOURCE, &sourceIP);
    inet_aton(DEST, &destIP);
    struct timeval timeout;
    bool stillSending = true;
    //queue all packets before sending?
    //if something to recv
        //recv and write it to a file
    //else
        //send the first packet in the queue and check again to see if there is something to recv
    
    struct packet queueEntry;
    queueEntry.next_hop = inp;
    struct timeval currentTime;
    gettimeofday(&currentTime, NULL);

    struct timeval delay;
    //delay will differ based on where the packet is going
    //just more info we need to parse from config file
    delay.tv_sec = 1; 
    timeradd(&delay, &currentTime, &queueEntry.deadLine);

    //timeout, send data
    printf("timeout, send data\n");
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
    ip_hdr.ip_ttl = 3;
    ip_hdr.ip_src = sourceIP;
    ip_hdr.ip_dst = destIP;
    struct ip* ip_header = &ip_hdr;
    //printf("before\n");
    //ip_header->ip_ttl = 50; //test constant
    //printf("after\n");
    //inet_ntoa((struct in_addr)ip_header->ip_src
    //ip_header.ip_src = inet_aton();
    memcpy(ipHeaderBuffer, ip_header, sizeof(struct ip));
    // //append headers to it somehow
    printf("%s\n", buffer);
    //strcat(ipHeaderBuffer, buffer);
    //printf("%s\n", ipHeaderBuffer);

    // UDP header
    int udpHeaderBufferSize = sizeof(struct udphdr);
    u_char udpHeaderBuffer[udpHeaderBufferSize];
    struct udphdr udp_hdr;
    udp_hdr.uh_sport = MYPORT;
    udp_hdr.uh_dport = MYPORT;
    struct udphdr* udp_header = &udp_hdr;

    int concatBufferSize = bufferSize + udpHeaderBufferSize + ipHeaderBufferSize;
    u_char concatBuffer[concatBufferSize];

    memcpy(concatBuffer, ipHeaderBuffer, ipHeaderBufferSize);
    memcpy(concatBuffer + ipHeaderBufferSize, udpHeaderBuffer, udpHeaderBufferSize);
    memcpy(concatBuffer + ipHeaderBufferSize + udpHeaderBufferSize, buffer, bufferSize);
    // sending IP things
    //send length of data first
    u_int32_t length = len + sizeof(struct ip) + sizeof(struct udphdr);  //length of all data in the buffer (data plus headers)
    u_int32_t* lengthP = &length;

    u_char* bufferCopy = (u_char*)malloc(bufferSize); 
    memcpy(bufferCopy, concatBuffer, concatBufferSize);
    //printf("before\n");
    queueEntry.buffer = (u_char*)malloc(bufferSize);
    //add buffer to packet
    memcpy(queueEntry.buffer, bufferCopy, concatBufferSize);
    //add length to packet
    queueEntry.bufferSize = concatBufferSize;
    queue.push(queueEntry);
    while(1) {
        gettimeofday(&currentTime, NULL);
        timeout.tv_sec = 0;
		timeout.tv_usec = 10000;
        FD_ZERO(&read_fd);
        FD_SET(sockfd, &read_fd);
        int r = select(sockfd + 1, &read_fd, NULL, NULL, &timeout);
        if(r == 0) {
            if(stillSending) {
                if(queue.size() > 0) {
                    //printf("looking in queue\n");
                    //if it's time to send the first packet, send it's length and then it's actual data
                    if(timercmp(&currentTime, &queue.front().deadLine, >=)){
                        printf("Attempting to send() length...\n");
                        if(cs3516_send(sockfd, &queue.front().bufferSize, sizeof(u_int32_t), queue.front().next_hop.s_addr) == -1) {
                            perror("error with sending the length");
                            exit(1);
                        }

                        //send data (with headers) over the network then pop the packet off the queue to move to the next
                        //if the queue is empty, we are done sending 
                        printf("Attempting to send() data...\n");

                        if(cs3516_send(sockfd, queue.front().buffer, queue.front().bufferSize, queue.front().next_hop.s_addr) == -1) {
                            perror("error with sending the concatenated data");
                            exit(1);
                        }
                        queue.pop();
                    }
                } else {
                    stillSending = false;   
                }
            }
        } else if(r < 0) {
            //error
            printf("error with select()\n");
        } else {
            //printf("%d\n", r);
            //receive data
            int bufferSize = 1000;
            u_char buffer[bufferSize];
            u_int32_t length;
            memset(buffer, 0, bufferSize);

            cs3516_recv(sockfd, buffer, bufferSize);
            //struct ip *ip_header = ((struct ip *) buffer);
            // printf("%s\n", buffer);
            // printf("%u\n", ip_header->ip_ttl);
            // printf("%s\n", inet_ntoa(ip_header->ip_src));
            // printf("%s\n", inet_ntoa(ip_header->ip_dst));

            printf("recieved the following\n");
            printf("%s\n", buffer + sizeof(struct ip) + sizeof(struct udphdr));
        }
            
    }

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
    //ip address that we want this process to be associated with
    //I think we can just have one machine run all of the routers
    //and then just change this hard coded IP for each router
    //in different directories
    //ROUTER 1 - "10.63.36.1"
    //ROUTER 2 - "10.63.36.12"
    //ROUTER 3 - "10.63.36.13"
    //server.sin_addr.s_addr = inet_addr(ROUTER_IP);
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
