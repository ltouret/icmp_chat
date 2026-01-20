#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
// #include <netinet/ip_icmp.h>
#include <arpa/inet.h>
#include <errno.h>

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

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        fprintf(stderr, "Usage: %s <destination_ip>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int sockfd;
    struct sockaddr_in dest_addr;

    sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_ICMP);
    if (sockfd < 0)
    {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    dest_addr.sin_family = AF_INET;
    if (inet_pton(AF_INET, argv[1], &dest_addr.sin_addr) <= 0)
    {
        perror("inet_pton");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    //! create loop that reads from stdin -> sendto -> recv -> repeat
    //! control c -> handle disconnect from server? -> maybe server pings to see if client still alive?
    struct icmp icmp_packet =
        {
            .username = "user",
            .roomname = "room",
            .message = "message",
            .icmp_type = ICMP_ECHO
        };

    if (sendto(sockfd, &icmp_packet, sizeof(icmp_packet), 0, (struct sockaddr *)&dest_addr, sizeof(dest_addr)) < 0)
    {
        perror("sendto");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    //! receive back from server -> doesnt work for now, we are catching our own echo the one that the kernel sends back not the one sent by our server
    unsigned char buffer[1000] = {0};
    ssize_t bytes_received = recvfrom(sockfd, buffer, sizeof(buffer), 0, NULL, NULL);
    printf("Received %zd bytes.\n", bytes_received);
    struct icmp *icmp_reply = (struct icmp *)(buffer); // Skip IP header

    if (icmp_reply->icmp_type == ICMP_ECHO_REPLY)
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
        printf("  %s\n", icmp_reply->username);
        printf("  %s\n", icmp_reply->roomname);
        printf("  %s\n", icmp_reply->message);
        // if (strcmp(icmp_reply->))
    }

    close(sockfd);
    return 0;
}