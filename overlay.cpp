//
// Created by Sidney Goldinger on 11/30/22.
//

////// INCLUDE STATEMENTS ///////
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <fstream>
#include <sstream>
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
#include <chrono>
using namespace std;

// Set the following port to a unique number:
#define NEXT_HOP "10.63.36.2" //temporary constant for testing
#define ROUTER "10.63.36.1" // for host to send to
string SOURCE = "1.2.3.1";
string DEST = "4.5.6.1";
int MYPORT = 5950;
#define ROUTER_IP "10.63.36.1" // for routers to bind to

////// CONFIG FILE GLOBALS: BOTH ////////
// line 1/2: (helps determine router/host id)
bool ID_FOUND = false;

string IP_ADDRESS = "-1";


/////// CONFIG FILE GLOBALS: EVERYTHING ////////
// steps and distances for each router/host
int DISTANCES [6][6];

// table for router IPs
/*
string ROUTER_IPS [6] = {"router_ip_1", "router_ip_2",
                         "router_ip_3", "router_ip_4", "router_ip_5",
                         "router_ip_6" };
                         */
string ROUTER_IPS [3] = {"router_ip_1", "router_ip_2",
                         "router_ip_3"};

// table for host IPs and fake IPs

string HOST_IPS [6] = {"host_ip_1", "host_ip_2",
                       "host_ip_3", "host_ip_4", "host_ip_5",
                       "host_ip_6" };

string HOST_FAKE_IPS [6] = {"host_fake_ip_1", "host_fake_ip_2",
                            "host_fake_ip_3", "host_fake_ip_4", "host_fake_ip_5",
                            "host_fake_ip_6" };

/////// CONFIG FILE GLOBALS: ROUTER ////////
// line 0:
int QUEUE_LENGTH = -1;

// line 1:
int ROUTER_ID = -1;

// line 3: (router-router connections)

// line 4: (router-host connections)


////// CONFIG FILE GLOBALS: HOST ///////
// line 0:
int TTL_VALUE = -1;

// line 2: (end-host setup)
int HOST_ID = -1;
string OVERLAY_IP = "default_overlay_ip";
bool OVERLAY_FOUND = false;

// line 4: (host-router connections)




/////// FUNCTION DECLARATIONS ///////
void router();
std::string random_string(const int);
void endhost();
int main(int, char**);
int create_cs3516_socket();
int cs3516_recv(int , void *, int);
int cs3516_send(int, void *, int, unsigned long);
void read_config_router();
void read_config_host();
void test_config_router();
void test_config_host();
void read_config_all();
void test_config_all();
bool router_host_connection(string, string);
string next_step(string, string);
int gimme_the_id(string);
int is_router(string);
string gimme_the_ip(int);
string gimme_real_ip(string);
int gimme_distance(string, string);
void do_the_log(string, string, int, string);
bool can_find_host(string);
void read_send_config();
string gimme_fake_ip(string);

struct packet {
    u_int32_t lengthOfFile;
    int bufferSize;
    struct timeval deadLine;
    struct in_addr next_hop;
    u_char* buffer;
};

void read_send_config() {
    // open file
    fstream configFile;
    configFile.open("send_config.txt",ios::in);
    if(configFile.is_open()) {
        string line;

        // go through lines
        while (getline(configFile, line)) {
            istringstream iss(line);
            bool is_first_num = true;
            int word_num = 0;

            // go through words in the line
            do {
                // get the word in the line
                string word;
                iss >> word;

                if (word_num == 0) {
                    DEST = word;
                }
                else if (word_num == 1) {
                    printf("%s\n", word.c_str());
                    MYPORT = stoi(word);
                }
                // we think that 3 is unnecessary

                word_num++;
            } while (iss);
        }
    }
}

bool can_find_host(string host_ip) {
    for (int i = 0; i < 6; i++) {
        if (HOST_IPS[i] == host_ip) { return true; }
    }
    return false;
}

void do_the_log(string source_overlay_ip, string dest_overlay_ip,
                int ip_ident, string status_code) {
    const auto p1 = std::chrono::system_clock::now();
    int unix_time = std::chrono::duration_cast<std::chrono::seconds>(p1.time_since_epoch()).count();

    std::ofstream outfile;

    outfile.open("ROUTER_control.txt", std::ios_base::app);
    outfile << unix_time << " " << source_overlay_ip << " " << dest_overlay_ip;
    outfile << " " << ip_ident << " " << status_code << "\n";
}

/**
 * gives the distance from starting to ending ip; Works!
 * @param starting_ip real ip starting
 * @param ending_ip real ip ending
 * @return the dist between in that direction
 */
int gimme_distance(string starting_ip, string ending_ip) {
    int starting_id = gimme_the_id(starting_ip);
    int ending_id = gimme_the_id(ending_ip);
    return(DISTANCES[starting_id][ending_id]);
}

/**
 * Takes a fake host ip, gives real ip of the host; UNTESTED
 * @param fake_ip fake host ip, string
 * @return real ip string of that host
 */
string gimme_real_ip(string fake_ip) {
    // fake ip
    int id = -1;
    for (int b = 0; b < 3; b++) {
        if (fake_ip == HOST_IPS[b]) { id = b; }
    }
    //cout << id << "\n";
    return HOST_IPS[id+3];
}

string gimme_fake_ip(string real_ip) {
    // fake ip
    int id = -1;
    for (int b = 3; b < 6; b++) {
        if (real_ip == HOST_IPS[b]) { id = b; }
    }
    //cout << id << "\n";
    return HOST_IPS[id-3];
}

/**
 * gives the ip of the next step the packet should go to towards the host; IT WORKS
 * @param starting_ip the ip starting from
 * @param host_ip the host ip going to
 * @return the ip of the next step router/host
 */
string next_step(string starting_ip, string host_ip) {
    int starting_id = gimme_the_id(starting_ip);
    int host_id = gimme_the_id(host_ip);

    if (is_router(starting_ip)) {
        // if the starting ip is a router, and it's connected to the host, return host ip
        if (router_host_connection(starting_ip, host_ip)) {
            return host_ip;
        }

            // if starting ip is a router, and not connected to the host, return the router ip
            //      connected to the host
        else {
            for (int a = 0; a < 3; a++) {
                if (DISTANCES[a][host_id] != -1) { // if this router is connected to host
                    return gimme_the_ip(a);
                }
            }
        }
    }
    else { // if starting ip is a host, return its router
        for (int b = 0; b < 3; b++) {
            if (DISTANCES[starting_id][b] != -1) {
                return gimme_the_ip(b);
            }
        }
    }
    return "balls"; // will never happen; to remove a warning
}

/**
 * Gets the ip of an unknown id; Works!
 * @param id the id we want the ip for
 * @return the ip of the id
 */
string gimme_the_ip(int id) {
    // if router
    for (int a = 0; a < 3; a++) {
        if (a == id) { return ROUTER_IPS[a]; }
    }
    // if host
    for (int b = 3; b < 6; b++) {
        if (b == id) { return HOST_IPS[b]; }
    }
    // else
    return "balls";
}

/**
 * Returns whether or not an IP is of a router or host; Works!
 * @param unknownIP the ip of the unknown thing
 * @return true if router; else, false
 */
int is_router(string unknownIP) {
    if (gimme_the_id(unknownIP) < 3) {
        return true;
    }
    else {
        return false;
    }
}

/**
 * Returns the id of an unknown IP; works!
 * @param unknownIP IP of what you want the ID of
 * @return the id of the ip
 */
int gimme_the_id(string unknownIP) {
    // check if router
    for (int a = 0; a < 3; a++) {
        if (unknownIP == ROUTER_IPS[a]) { return a; }
    }

    // check if host
    for (int b = 3; b < 6; b++) {
        if (unknownIP == HOST_IPS[b]) { return b; }
    }

    // else
    return -1;
}

/**
 * Tests whether a router and host are directly connected. Works!
 * @param router_ip the (real) ip of a router
 * @param host_ip the (real) ip of a host
 * @return whether or not they are directly connected
 */
bool router_host_connection(string router_ip, string host_ip) {
    // get ids of each
    // router
    int router_id = -1;
    for (int a = 0; a < 3; a++) {
        if (ROUTER_IPS[a] == router_ip) { router_id = a; }
    }

    // host
    int host_id = -1;
    for (int b = 3; b < 6; b++) {
        //cout << HOST_IPS[b] << ", " << host_ip << "\n";
        if (0 == HOST_IPS[b].compare(host_ip)) { host_id = b; }
    }
    //cout << "Router id, host id: " << router_id << ", " << host_id << "\n";

    // check if there's a distance in DISTANCES
    if ((DISTANCES[host_id][router_id] != -1) || (DISTANCES[router_id][host_id] != -1)) {
        //cout << DISTANCES[host_id][router_id] << " ";
        //cout << DISTANCES[router_id][host_id];

        //cout << "\nThey connect!\n";
        return true;
    }
    else { return false; }

    return true;
}

/**
 * Prints the data read in from send_config as all; does connection distance and all id things
 */
void test_config_all() {
    cout << "Distances: \n";
    for (int a = 0; a < 6; a++) {
        for (int b = 0; b < 6; b++) {
            cout << "Starting: " << a << ", going: " << b << ", distance = " << DISTANCES[a][b] << "\n";
        }
    }

    cout << "\nRouter IPs: \n";
    for (int c = 0; c < 3; c++) {
        cout << "Router " << c << " has IP " << ROUTER_IPS[c] << "\n";
    }

    cout << "\nHost IPs: \n";
    for (int d = 0; d < 6; d++) {
        cout << "Host " << d << " has IP " << HOST_IPS[d] << "\n";
    }
}

/**
 * Prints the data read in from send_config as a router
 */
void test_config_router() {
    cout << "QUEUE_LENGTH: " << QUEUE_LENGTH << "\n";
    cout << "ROUTER_ID: " << ROUTER_ID << "\n";
}

/**
 * Prints the data read in from send_config as a host
 */
void test_config_host() {
    cout << "TTL_VALUE: " << TTL_VALUE << "\n";
    cout << "HOST_ID: " << HOST_ID << "\n";
    cout << "OVERLAY_IP: " << OVERLAY_IP << "\n";
}

/**
 * Reads in send_config.txt if router; sets globals accordingly
 */
void read_config_router() {
    // open file
    fstream configFile;
    configFile.open("config.txt",ios::in);
    if(configFile.is_open()) {
        string line;

        // go through lines
        while(getline(configFile, line)) {
            istringstream iss(line);
            bool is_first_num = true;
            int first_num = -1;
            int word_num = 0;

            // go through words in the line
            do {
                // get the word in the line
                string word;
                iss >> word;

                if (is_first_num) { // get first number
                    //cout << "is first num: yes! num = " << word << "\n";
                    first_num = stoi(word);
                    //cout << "does " << word << " equal " << first_num << "?\n";
                    is_first_num = false;
                }
                else { // if not first, figure out which
                    word_num = word_num + 1;
                    //cout << "not first; word_num = " << word_num << "\n";
                }

                // now switch case first number to determine what it means and do that
                // note: last "word" in each line is empty (no characters)
                switch (first_num) {
                    case 0:
                        //cout << "Line 0. Queue length setting!\n";
                        //cout << "word: " << word << "\n";
                        //cout << "word_num: " << word_num << "\n";

                        // if word_num = 0, it is 0
                        // if word_num = 1, word = QUEUE_LENGTH
                        // if word_num = 2, ignore (end-host thing)
                        if (word_num == 1) { QUEUE_LENGTH = stoi(word); }

                        break;
                    case 1:
                        //cout << "Line 1. Setting up router id.\n";
                        //cout << "word: " << word << "\n";
                        //cout << "word_num: " << word_num << "\n";

                        // if word_num = 0, it is 1
                        // if word_num = 1, if !ID_FOUND, ROUTER_ID = word
                        // if word_num = 2, if !ID_FOUND and word = real IP addr (ROUTER?), ID_FOUND = true
                        if (word_num == 1) {
                            if (!ID_FOUND) { ROUTER_ID = stoi(word); }
                        }
                        //else if (word_num == 2) {
                        //    if ((!ID_FOUND) && (word == ROUTER)) { ID_FOUND = true; }
                        //}

                        break;
                    case 3:
                        //cout << "Line 3. Setting up router connection.\n";
                        //cout << "word: " << word << "\n";
                        //cout << "word_num: " << word_num << "\n";
                        break;
                    case 4:
                        //cout << "Line 4. Setting up router-host connection.\n";
                        //cout << "word: " << word << "\n";
                        //cout << "word_num: " << word_num << "\n";
                        break;
                        //default:
                        //cout << "Not a relevant line. line = " << first_num << "\n";
                        //cout << "word: " << word << "\n";
                        //cout << "word_num: " << word_num << "\n";

                }
            } while (iss);

        }
    }
}

/**
 * Reads in send_config.txt if host; sets globals accordingly
 */
void read_config_host() {
    // open file
    fstream configFile;
    configFile.open("config.txt",ios::in);
    if(configFile.is_open()) {
        string line;
        while(getline(configFile, line)) {
            istringstream iss(line);
            bool is_first_num = true;
            int first_num = -1;
            int word_num = 0;

            // go through words in the line
            do {
                // get the word in the line
                string word;
                iss >> word;

                if (is_first_num) { // get first number
                    //cout << "is first num: yes! num = " << word << "\n";
                    first_num = stoi(word);
                    //cout << "does " << word << " equal " << first_num << "?\n";
                    is_first_num = false;
                }
                else { // if not first, figure out which
                    word_num = word_num + 1;
                    //cout << "not first; word_num = " << word_num << "\n";
                }

                // now switch case first number to determine what it means and do that
                // note: last "word" in each line is empty (no characters)
                switch (first_num) {
                    case 0:
                        //cout << "Line 0. TTL setting!\n";
                        //cout << "word: " << word << "\n";
                        //cout << "word_num: " << word_num << "\n";

                        // if word_num = 0, it is 0
                        // if word_num = 1, ignore (router thing)
                        // if word_num = 2, set TTL_VALUE
                        if (word_num == 2) { TTL_VALUE = stoi(word); }

                        break;
                    case 2:
                        //cout << "Line 2. Setting up host id.\n";

                        // if word_num = 0, it is 1
                        // if word_num = 1, if !ID_FOUND, HOST_ID = word
                        // if word_num = 2, if !ID_FOUND and word = real IP addr (ROUTER?), ID_FOUND = true
                        // TODO: What is the IP Addr if it's a host?
                        // if word_num = 3, if ID_FOUND, OVERLAY_IP = word
                        if (word_num == 1) {
                            if (!ID_FOUND) { HOST_ID = stoi(word); }
                        }
                        //if (word_num == 2) {
                        //    if ((!ID_FOUND) && (ROUTER == word)) { ID_FOUND = true; }
                        //}
                        if (word_num == 3) {
                            if ((ID_FOUND) && (OVERLAY_FOUND == false)) {
                                OVERLAY_IP = word;
                                OVERLAY_FOUND = true;
                            }
                        }

                        break;
                    case 4:
                        //cout << "Line 4. Setting up host-router connection.\n";

                        break;
                        //default:
                        //cout << "Not a relevant line. line = " << first_num << "\n";
                        //cout << "word: " << word << "\n";
                        //cout << "word_num: " << word_num << "\n";

                }

            } while (iss);

        }
    }

}

void read_config_all() {
    // set variables for iteration
    int num_in_table = 0;
    int num_in_table_two = 0;

    // variables for 3
    int first_thing = -1;
    int second_thing = -1;
    int first_distance = -10;
    int second_distance = -10;


    // fill DISTANCES with -1
    for (int a = 0; a < 6; a++) {
        for (int b = 0; b < 6; b++) {
            DISTANCES[a][b] = -1;
        }
    }
    // variables for 4

    //cout << "got to here.\n";


    // open file
    fstream configFile;
    configFile.open("config.txt",ios::in);
    if(configFile.is_open()) {
        string line;
        while(getline(configFile, line)) {
            istringstream iss(line);
            bool is_first_num = true;
            int first_num = -1;
            int word_num = 0;

            // go through words in the line
            do {
                // get the word in the line
                string word;
                iss >> word;

                if (is_first_num) { // get first number
                    first_num = stoi(word);
                    is_first_num = false;
                }
                else { // if not first, figure out which
                    word_num = word_num + 1;
                }

                // now switch case first number to determine what it means and do that
                // note: last "word" in each line is empty (no characters)
                switch (first_num) {
                    case 0:
                        // irrelevant?
                        break;
                    case 1:
                        // routers and IDs
                        if (word_num == 2) {
                            ROUTER_IPS[num_in_table] = word;
                            num_in_table = num_in_table + 1;
                            if (num_in_table == 6) { num_in_table = 0; }
                        }

                        break;
                    case 2:
                        // hosts and IDs and fake IDs
                        if (word_num == 2) {
                            HOST_IPS[num_in_table] = word;
                            num_in_table = num_in_table + 1;
                            if (num_in_table == 6) { num_in_table = 0; }
                        }
                        if (word_num == 3) {
                            HOST_IPS[num_in_table_two] = word;
                            num_in_table_two = num_in_table_two + 1;
                            if (num_in_table_two == 6) { num_in_table_two = 0; }
                        }

                        break;
                    case 3:
                        // router distances
                        if (word_num == 1) { first_thing = stoi(word); }
                        if (word_num == 2) { first_distance = stoi(word); }
                        if (word_num == 3) { second_thing = stoi(word); }
                        if (word_num == 4) {
                            second_distance = stoi(word);

                            // now set the things
                            DISTANCES[first_thing-1][second_thing-1] = first_distance;
                            DISTANCES[second_thing-1][first_thing-1] = second_distance;
                        }


                        break;
                    case 4: // TODO haven't dealt with fake IP/number thing
                        // host distances
                        // router distances
                        if (word_num == 1) { first_thing = stoi(word); }
                        if (word_num == 2) { first_distance = stoi(word); }
                        if (word_num == 4) { second_thing = stoi(word); }
                        if (word_num == 5) {
                            second_distance = stoi(word);

                            // now set the things
                            //cout << "first_thing = " << first_thing << "\n";
                            //cout << "second_thing = " << second_thing << "\n";
                            //cout << "first_distance = " << first_distance << "\n";
                            //cout << "second_distance = " << second_distance << "\n";

                            DISTANCES[first_thing-1][second_thing-1] = first_distance;
                            DISTANCES[second_thing-1][first_thing-1] = second_distance;
                        }

                        break;
                        //default:
                        //cout << "not one of the cases.\n";
                }

            } while (iss);

        }
    }

}

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
    int maxQueueSize = QUEUE_LENGTH; //read from config file
    printf("Queue length: %d\n", maxQueueSize);
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
    int bufferSize = 1000 + sizeof(struct ip) + sizeof(struct udphdr);
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
            //timeout, check the queue
            //printf("checking the queue\n");
            if(queue.size() > 0) {
                //printf("%s\n", queue.front().buffer + sizeof(struct ip) + sizeof(struct udphdr));
                //printf("%d\n", queue.front().bufferSize);
                //printf("%s\n", inet_ntoa(queue.front().next_hop));
                if(timercmp(&currentTime, &queue.front().deadLine, >=)){
                    printf("Forwarding! (to: %s)\n", inet_ntoa(queue.front().next_hop));
                    struct ip *ip_header = ((struct ip *) queue.front().buffer);
                    std::string sourceLog(inet_ntoa(ip_header->ip_src));
                    std::string destLog(inet_ntoa(ip_header->ip_dst));
                    do_the_log(sourceLog, destLog, ip_header->ip_id, "SEND_OKAY");

                    //also send length of file here
                    if(cs3516_send(sockfd, &queue.front().lengthOfFile, sizeof(u_int32_t), queue.front().next_hop.s_addr) == -1) {
                        perror("error with sending the length");
                        exit(1);
                    }
                    cs3516_send(sockfd, queue.front().buffer, queue.front().bufferSize, queue.front().next_hop.s_addr);
                    printf("queue size before pop: %ld\n", queue.size());
                    queue.pop();
                    printf("queue size after pop: %ld\n", queue.size());
                }
            }
        } else if(r < 0) {
            //error
            printf("error\n");
        } else {
            printf("queue size: %ld\n", queue.size());
            printf("mqx size: %d\n", maxQueueSize);
            if(queue.size() < maxQueueSize) {
                //printf("tyring to recv()\n");
                //ready to recvfrom() something
                //recv length, data, put into a packet struct, push to queue
                struct packet queueEntry;
                //https://stackoverflow.com/questions/19555121/how-to-get-current-timestamp-in-milliseconds-since-1970-just-the-way-java-gets

                //printf("current time (ms): %ld\n", ms);
                // recieving IP and UDP headers n shit
                //printf("Attempting to recv() length...\n");
                memset(buffer, 0, bufferSize);

                //length will be the total size of the file being sent (in multiple packets most likely)
                cs3516_recv(sockfd, &length, sizeof(u_int32_t));
                //printf("%d\n", length);
                queueEntry.lengthOfFile = length;
                //allocate enough for the maximum payload of 1000 bytes plus the size of the headers
                queueEntry.bufferSize = 1000 + sizeof(struct ip) + sizeof(struct udphdr);

                //printf("Attempting to recv() data...\n");
                memset(buffer, 0, bufferSize);
                cs3516_recv(sockfd, buffer, bufferSize);
                struct ip *ip_header = ((struct ip *) buffer);
                ip_header->ip_ttl--;
                //printf("%s\n", buffer);
                //printf("%u\n", ip_header->ip_ttl);
                std::string source(inet_ntoa(ip_header->ip_src));
                std::string dest(inet_ntoa(ip_header->ip_dst));

                //print data (won't always make sense to print it this way)
                //printf("%s\n", buffer + sizeof(struct ip) + sizeof(struct udphdr));
                u_char* bufferCopy = (u_char*)malloc(bufferSize);
                memcpy(bufferCopy, buffer, bufferSize);
                queueEntry.buffer = (u_char*)malloc(bufferSize);
                memcpy(queueEntry.buffer, bufferCopy, bufferSize);
                //printf("%s\n", queueEntry.buffer + sizeof(struct ip) + sizeof(struct udphdr));


                struct timeval delay;
                //delay will differ based on where the packet is going
                //just more info we need to parse from config file
                std::string str(inet_ntoa(ip_header->ip_dst));
                long int delayMs = gimme_distance(IP_ADDRESS, next_step(IP_ADDRESS, gimme_real_ip(str)));
                printf("delay in ms: %ld\n", delayMs);
                delay.tv_sec = 10;
                delay.tv_usec = delayMs * 1000;
                printf("%ld\n", delay.tv_usec);
                


                timeradd(&delay, &currentTime, &queueEntry.deadLine);


                printf("ttl: %d\n", ip_header->ip_ttl);
                if (ip_header->ip_ttl > 0) {
                    //if (strcmp(inet_ntoa(ip_header->ip_dst), "4.5.6.1") == 0) {
                        //send to real address of "4.5.6.1"
                        struct in_addr next;
                        std::string str(inet_ntoa(ip_header->ip_dst));
                        if(can_find_host(str)) {
                            inet_aton(next_step(IP_ADDRESS, gimme_real_ip(str)).c_str(), &next);
                            // printf("Enqueuing\n");
                            // struct ip* ipHeader = (struct ip*)queueEntry.buffer;
                            // std::cout << str << "\n";
                            // std::cout << gimme_real_ip(str) << "\n";
                            // std::cout << next_step(ROUTER_IP, gimme_real_ip(str)) << "\n";
                            queueEntry.next_hop = next;
                            //cs3516_send(sockfd, buffer, bufferSize, next.s_addr);
                            //}
                            //else {
                                //else send to the other option
                            //     printf("Enqueuing for other dest\n");
                            //     struct in_addr next;
                            //     inet_aton("10.63.36.4", &next);
                            //     queueEntry.next_hop = next;
                            //     //cs3516_send(sockfd, buffer, bufferSize, next.s_addr);
                            // }
                            queue.push(queueEntry);
                        } else {
                            std::string sourceLog(inet_ntoa(ip_header->ip_src));
                            std::string destLog(inet_ntoa(ip_header->ip_dst));
                            do_the_log(sourceLog, destLog, ip_header->ip_id, "NO_ROUTE_TO_HOST");
                        }
                }
                else { // if it's zero, log
                    printf("packet dropped beacuse ttl was <= 0\n");
                    std::string sourceLog(inet_ntoa(ip_header->ip_src));
                    std::string destLog(inet_ntoa(ip_header->ip_dst));
                    do_the_log(sourceLog, destLog, ip_header->ip_id, "TTL_EXPIRED");
                }
            } else {
                //recv the data so that more can be recvd
                cs3516_recv(sockfd, &length, sizeof(u_int32_t));
                memset(buffer, 0, bufferSize);
                cs3516_recv(sockfd, buffer, bufferSize);
                struct ip *ip_header = ((struct ip *) buffer);
                std::string sourceLog(inet_ntoa(ip_header->ip_src));
                std::string destLog(inet_ntoa(ip_header->ip_dst));
                do_the_log(sourceLog, destLog, ip_header->ip_id, "MAX_SENDQ_EXCEEDED");
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
    inet_aton(SOURCE.c_str(), &sourceIP);
    inet_aton(DEST.c_str(), &destIP);
    struct timeval timeout;
    bool stillSending = true;

    //just a bunch of digits of pi for now
    FILE* body = fopen("send_body.txt", "r");
    FILE* received = fopen("received", "w");
    //FILE* test = fopen("bytesRead", "w");

    //find the size of the file we are going to send
    fseek(body, 0, SEEK_END);
    u_int32_t length = ftell(body);
    printf("length of file to transmit: %d\n", length);
    u_int32_t* lengthP = &length;
    rewind(body);



    int numPacketsToSend = (length/1000) + 1;
    int numPacketsToReceive = 0;
    int highestPacketReceived = 0;
    bool* packetHasArrived;
    struct timeval currentTime;


    printf("num packets to transmit: %d\n", numPacketsToSend);
    for(int i = 0; i < numPacketsToSend; i ++) {
        struct packet queueEntry;
        struct in_addr next;
        std::string destination(DEST);
        inet_aton(next_step(IP_ADDRESS, destination).c_str(), &next);
        queueEntry.next_hop = next;
        if(queue.size() == 0) {
            gettimeofday(&currentTime, NULL);
        } else {
            currentTime = queue.back().deadLine;
        }

        struct timeval delay;
        //delay will differ based on where the packet is going
        //just more info we need to parse from config file
        printf("host source: %s\n", IP_ADDRESS.c_str());
        printf("host dest: %s\n", inet_ntoa(queueEntry.next_hop));
        std::string ending_ip(inet_ntoa(queueEntry.next_hop));
        printf("host delay: %d\n", gimme_distance(IP_ADDRESS, ending_ip));
        delay.tv_sec = 0;
        delay.tv_usec = 1000*gimme_distance(IP_ADDRESS, ending_ip);
        timeradd(&delay, &currentTime, &queueEntry.deadLine);

        int bufferSize = 1000; //send 1000 bytes of the file at a time
        char buffer[bufferSize];

        //clear buffer just in case I guess
        memset(buffer, 0, bufferSize);

        //random string stuff I don't think we need anymore
        // int len = 20;
        // std::string random_string_std = random_string(len);
        // char* rand_str = const_cast<char*>(random_string_std.c_str()); // converting std::string to char *
        // //std::cout << rand_str << "\n"; // testing the random string generator (it works tho)
        // strcpy(buffer, rand_str);

        //read 1000 bytes into the buffer at a time from the file body
        long int bytesRead = fread(buffer, 1, 1000, body);
        printf("byetes read: %ld\n", bytesRead);

        //construct headers
        // IP header
        int ipHeaderBufferSize = sizeof(struct ip);
        u_char ipHeaderBuffer[ipHeaderBufferSize];
        struct ip ip_hdr;
        ip_hdr.ip_ttl = 3;               //from config
        if(i == 3) {
            ip_hdr.ip_ttl = 1;           //intentionally drop the fourth packet for testing
        }
        ip_hdr.ip_src = sourceIP;
        ip_hdr.ip_dst = destIP;          //from send_config
        ip_hdr.ip_id = i;               //increase for each consequtive packet
        struct ip* ip_header = &ip_hdr;
        //copy header struct into the buffer
        memcpy(ipHeaderBuffer, ip_header, sizeof(struct ip));

        // UDP header
        int udpHeaderBufferSize = sizeof(struct udphdr);
        u_char udpHeaderBuffer[udpHeaderBufferSize];
        struct udphdr udp_hdr;
        udp_hdr.uh_sport = MYPORT;                       //from send_config
        udp_hdr.uh_dport = MYPORT;                       //from send_config
        struct udphdr* udp_header = &udp_hdr;

        int concatBufferSize = bufferSize + udpHeaderBufferSize + ipHeaderBufferSize;
        u_char concatBuffer[concatBufferSize];

        //copy ipHeader data into the first portion of the concat buffer
        memcpy(concatBuffer, ipHeaderBuffer, ipHeaderBufferSize);
        //then copy udpHeader data into the memory right after the ipHeader data
        memcpy(concatBuffer + ipHeaderBufferSize, udpHeaderBuffer, udpHeaderBufferSize);
        //finally copy the actual file data to the end of the buffer
        memcpy(concatBuffer + ipHeaderBufferSize + udpHeaderBufferSize, buffer, bufferSize);
        // sending IP things
        //send length of data first

        u_char* bufferCopy = (u_char*)malloc(concatBufferSize);
        memcpy(bufferCopy, concatBuffer, concatBufferSize);
        //printf("before\n");
        queueEntry.buffer = (u_char*)malloc(concatBufferSize);
        //add buffer to packet
        memcpy(queueEntry.buffer, bufferCopy, concatBufferSize);
        //add length to packet
        queueEntry.bufferSize = bytesRead + sizeof(struct ip) + sizeof(struct udphdr);  //used to be  " = concatBufferSize"
        queue.push(queueEntry);
    }

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
                        if(cs3516_send(sockfd, &length, sizeof(u_int32_t), queue.front().next_hop.s_addr) == -1) {
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
            int bufferSize = 1000 + sizeof(struct ip) + sizeof(struct udphdr);
            u_char buffer[bufferSize];
            u_int32_t length;
            memset(buffer, 0, bufferSize);

            //also recv length of whole file here
            //memset(buffer, 0, bufferSize);

            //length will be the total size of the file being sent (in multiple packets most likely)
            cs3516_recv(sockfd, &length, sizeof(u_int32_t));
            cs3516_recv(sockfd, buffer, bufferSize);

            struct ip *ip_header = ((struct ip *) buffer);
            if(ip_header->ip_id == 0) {
                numPacketsToReceive = (length/1000) + 1;
                highestPacketReceived = 0;
                packetHasArrived = (bool *)malloc(sizeof(bool) * numPacketsToReceive);
                packetHasArrived[0] = true;
                printf("packet num: %d\n", highestPacketReceived);
                //resest file/make new file?
            } else {
                highestPacketReceived = ip_header->ip_id;
                packetHasArrived[highestPacketReceived] = true;
                printf("packet num: %d\n", highestPacketReceived);
            }

            // printf("%s\n", buffer);
            // printf("%u\n", ip_header->ip_ttl);
            // printf("%s\n", inet_ntoa(ip_header->ip_src));
            // printf("%s\n", inet_ntoa(ip_header->ip_dst));

            //printf("recieved the following\n");
            fwrite(buffer + sizeof(struct ip) + sizeof(struct udphdr), 1, 1000, received);
            printf("recieved a file size of: %d\n", length);
            if(highestPacketReceived == numPacketsToReceive-1) {
                fclose(received);
                for(int i = 0; i < numPacketsToReceive; i ++) {
                    if(!packetHasArrived[i]) {
                        printf("Packet with ident #: %d lost!\n", i);
                    }
                }
            }
        }

    }

}

int main (int argc, char **argv) {


    // TODO TESTING REMOVE THIS AFTER
    //read_config_router();
    //test_config_router();

    //read_config_host();
    //test_config_host();

    read_send_config();
    cout << SOURCE << "\n";
    cout << DEST << "\n";
    //cout << "TESTING \n\n";
    read_config_all();
    read_config_router();
    read_config_host();
    //read_send_config();
    //cout << DEST << " " << SOURCE << " ";

    //test_config_all();
    //cout << gimme_distance("10.0.2.103","10.0.2.101") << "\n";

    //return 0;

    // TODO END TESTING


    // test if end-host or router
    std::string arg1 = argv[1];

    if (stoi(arg1) <= 2) {
        ROUTER_ID = stoi(arg1);
        IP_ADDRESS = gimme_the_ip(ROUTER_ID);
        printf("router ip address: %s\n", IP_ADDRESS.c_str());
        router();
    }
    else if (stoi(arg1) >= 3 && stoi(arg1) <= 5) {
        HOST_ID = stoi(arg1);
        IP_ADDRESS = gimme_the_ip(HOST_ID);
        printf("host ip address: %s\n", IP_ADDRESS.c_str());
        endhost();
    }
    else {
        cout << "There are only 6 routers/hosts. Input a number between 0 and 5, inclusive.\n";
    }

    /*
    if (arg1 == "r") { router(); }
    else if (arg1 == "h") { endhost(); }
    else { printf("please input r or h.\n"); return 0; }
     */

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
    server.sin_addr.s_addr = inet_addr(IP_ADDRESS.c_str());
    //server.sin_addr.s_addr = "10.63.36.13";
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

