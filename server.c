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
#define ICMP_ECHO_REPLY 0

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
//? too big?
struct room
{
    char roomname[11];
    char users[100][11]; //! maybe transform users to ints? so i just keep 100 ints here? -> user sam logs in becomes 1, etc
    struct room *next;
};

void create_room(struct room **room, const char *roomname)
{
    if (*room == NULL)
    {
        // malloc
        *room = malloc(sizeof(struct room));
        strcpy((*room)->roomname, roomname);
        (*room)->next = NULL;
        printf("room created with roomname: %s\n", (*room)->roomname);
        // printf("%ld\n", sizeof(struct room));
        return;
    }
    // if roomname exists done
    struct room *current = *room;
    while (current->next)
    {
        if (current->roomname == roomname)
        {
            return;
        }
        current = current->next;
    }

    // printf("current is %s\n", current->roomname);
    // else create
    // malloc and add to linked list
    current->next = malloc(sizeof(struct room));
    // current->next-> = malloc(sizeof(struct room));
    strcpy(current->next->roomname, roomname);
    current->next->next = NULL;
    // printf("current after %s\n", current->next->roomname);
}

unsigned short calculate_checksum(unsigned short *ptr, int nbytes) {
    long sum;
    unsigned short oddbyte;
    short answer;

    sum = 0;
    while (nbytes > 1) {
        sum += *ptr++;
        nbytes -= 2;
    }
    if (nbytes == 1) {
        oddbyte = 0;
        *((u_char *)&oddbyte) = *(u_char *)ptr;
        sum += oddbyte;
    }

    sum = (sum >> 16) + (sum & 0xffff);
    sum = sum + (sum >> 16);
    answer = (short)~sum;

    return (answer);
}

int main()
{
    int sockfd;
    struct room *room = NULL;
    create_room(&room, "room");
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
            //! change the name of this or use it only to get the ehader not all the data!
            struct icmp *icmp_header = (struct icmp *)(buffer + ip_header->ihl * 4); // Skip IP header

            // You can further process the ICMP data here
            if (icmp_header->icmp_type == ICMP_ECHO)
            {
                printf("Received %zd bytes.\n", bytes_received);

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
                // printf("  %d\n", icmp_header->icmp_id);
                printf("  %s\n", icmp_header->message);

                //! add packet validation to see if it comes from a client, maybe create a checksum function? -> could add it to icmp_header->padding, 
                //! like that i know when its a ping of mine, and for all else i can do a normal ping reply
                if (!strcmp(icmp_header->roomname, "room"))
                    printf("same room, validation ok\n");

                //! reply back to the clients, infinite loop for now bc we reply and then our recv receives it as we are using the same ip address for server and client
                struct sockaddr_in dest_addr;
                dest_addr.sin_family = AF_INET;
                if (inet_pton(AF_INET, inet_ntoa(*(struct in_addr *)&ip_header->saddr), &dest_addr.sin_addr) <= 0)
                {
                    perror("inet_pton");
                    close(sockfd);
                    exit(EXIT_FAILURE);
                }
                struct icmp icmp_packet =
                    {
                        .username = "server",
                        .roomname = "room",
                        .message = "i am the server!",
                        .icmp_id = icmp_header->icmp_id,
                        .icmp_type = ICMP_ECHO_REPLY
                    };

                icmp_packet.icmp_cksum = 0;
                icmp_packet.icmp_cksum = calculate_checksum((unsigned short *)&icmp_packet, sizeof(icmp_packet));
                if (sendto(sockfd, &icmp_packet, sizeof(icmp_packet), 0, (struct sockaddr *)&dest_addr, sizeof(dest_addr)) < 0)
                {
                    perror("sendto");
                    close(sockfd);
                    exit(EXIT_FAILURE);
                }
                printf("sent\n");
            }
        }
    }

    close(sockfd);
    return 0;
}