#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
// #include <netinet/ip_icmp.h>
#include <netinet/ip.h>
#include <arpa/inet.h>

//! add defines for max len of username, roomname, max rooms etc, no magic numbers
// # define ICMP_
#define ICMP_ECHO 8

struct icmp
{
    // header 20 bytes
    uint8_t icmp_type;
    uint8_t icmp_code;
    uint16_t icmp_cksum; // padding
    uint16_t icmp_id;
    uint16_t icmp_seq;
    uint32_t icmp_otime;
    uint32_t icmp_rtime; // padding
    uint32_t icmp_ttime;

    // padding 4 bytes
    uint32_t padding;

    // data 40 bytes
    // uint8_t data[40];
    uint8_t username[10];
    uint8_t roomname[10];
    //! try with more len for message
    uint8_t message[20];
};

//! maybe max rooms to not get ddos out of memory by some random?
struct room
{
    char roomname[11];
    char users[100][11]; //! maybe transform users to ints? so i just keep 100 ints here? -> user sam logs in becomes 1, etc
    struct room *next;
};


//! create room if it doesnt exist
void create_room(struct room *room, const char *roomname)
{
    if (room == NULL)
    {
        //malloc
    }
    // if roomname exists done
    struct room *current = room;
    while (current)
    {
        if (current->roomname == roomname)
        {
            break;
        }
        current = current->next;
    }
    // else create
    // malloc and add to linked list
}

int main()
{
    int sockfd;
    struct room *room = NULL;
    unsigned char buffer[1000] = {0};

    sockfd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
    if (sockfd < 0)
    {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    while (1)
    {
        ssize_t bytes_received = recvfrom(sockfd, buffer, sizeof(buffer), 0, NULL, NULL);
        if (bytes_received < 0)
        {
            perror("recvfrom");
            continue; // Or handle the error as needed
        }

        // Parse the IP header
        struct iphdr *ip_header = (struct iphdr *)buffer;

        // Verify that the packet is ICMP
        if (ip_header->protocol == IPPROTO_ICMP)
        {
            // Parse the ICMP header
            struct icmp *icmp_header = (struct icmp *)(buffer + ip_header->ihl * 4); // Skip IP header

            // You can further process the ICMP data here
            if (icmp_header->icmp_type == ICMP_ECHO)
            {
                // printf("Received %zd bytes.\n", bytes_received);

                // printf("IP Header:\n");
                // printf("  Version: %u\n", ip_header->version);
                // printf("  Header Length: %u\n", ip_header->ihl * 4);
                // printf("  Source IP: %s\n", inet_ntoa(*(struct in_addr *)&ip_header->saddr));
                // printf("  Destination IP: %s\n", inet_ntoa(*(struct in_addr *)&ip_header->daddr));
                // printf("  Protocol: %u\n", ip_header->protocol);

                printf("ICMP data:\n");
                // printf("  Type: %u\n", icmp_header->icmp_type);
                // printf("  Code: %u\n", icmp_header->icmp_code);
                printf("  %s\n", icmp_header->username);
                printf("  %s\n", icmp_header->roomname);
                printf("  %s\n", icmp_header->message);

                //! if room doesnt exist create
            }
        }
    }

    close(sockfd);
    return 0;
}