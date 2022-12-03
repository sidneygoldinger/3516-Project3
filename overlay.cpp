//
// Created by Sidney Goldinger on 11/30/22.
//

#include <stdio.h>
#include <iostream>
#include <string_view>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <string.h>
#include <arpa/inet.h>

// Set the following port to a unique number:
#define MYPORT 5950

void router();
void endhost();
int main(int, char**);
int create_cs3516_socket();
int cs3516_recv(int , char *, int);
int cs3516_send(int, char *, int, unsigned long);


void router() {
    //printf("router\n");
    printf("Attempting to create a socket...\n");
    int sockfd = create_cs3516_socket();
    int bufferSize = 64;
    char buffer[bufferSize];
    printf("Attempting to recv()...\n");
    cs3516_recv(sockfd, buffer, bufferSize);
    printf("%s\n", buffer);
}

void endhost() {
    printf("host\n");
    printf("Attempting to create a socket...\n");
    int sockfd = create_cs3516_socket();
    int bufferSize = 64;
    char buffer[bufferSize];
    strcpy(buffer, "test string");
    printf("Attempting to send()...\n");
    struct in_addr inp;
    inet_aton("10.63.36.1", &inp);
    cs3516_send(sockfd, buffer, bufferSize, inp.s_addr);
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

int cs3516_recv(int sock, char *buffer, int buff_size) {
    struct sockaddr_in from;
    int fromlen, n;
    fromlen = sizeof(struct sockaddr_in);
    n = recvfrom(sock, buffer, buff_size, 0,
                 (struct sockaddr *) &from, (socklen_t*)&fromlen);

    return n;
}

int cs3516_send(int sock, char *buffer, int buff_size, unsigned long nextIP) {
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
