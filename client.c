#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
// #include <netinet/ip_icmp.h>
#include <arpa/inet.h>
#include <errno.h>
#include <poll.h>

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


/*
    For client we want to receive data from server and stdin at the same time,
    so we are going to change this and use poll to listen to those two entry points!
    update it :D
    now we will mae this work like this:
    ./client username server_ip
*/
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

    //! add protect against invalid ip address
    dest_addr.sin_family = AF_INET;
    if (inet_pton(AF_INET, argv[1], &dest_addr.sin_addr) <= 0)
    {
        perror("inet_pton");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    struct pollfd fds[2];
    fds[0].fd = STDIN_FILENO;  // Watch keyboard
    fds[0].events = POLLIN;
    fds[1].fd = sockfd;     // Watch socket
    fds[1].events = POLLIN;

    while (1) {
        poll(fds, 2, -1); // Wait forever until something happens

        // Keyboard has data! Read it and send() over socket.
        if (fds[0].revents & POLLIN) {
            //! no magic numbers add a define with max 20
            // ! add more buffer for the msgs and add some kind of buffer split
            // ! than when its more 100 chars we send it on the next message instead of loosing the message
            // ? Example buffer 5 -> Hello -> first msg -> World -> second msg -> ! -> third msg
            char msg_buffer[20] = {0};
            
            // For now if we get a msg while user is writing then it will scramble the terminal screen
            // By printing the message in the middle of the msg the user was going to send. Fix it!
            fgets(msg_buffer, 20, stdin); //? <- receive direclty in icmp_packet.message to have to copy less data around?
            msg_buffer[strcspn(msg_buffer, "\n")] = '\0'; // Remove newline character
            printf("\033[A\33[2K\r"); 
            fflush(stdout);
            // printf("we sent %s", msg_buffer);
            //! create loop that reads from stdin -> sendto -> recv -> repeat
            //! control c -> handle disconnect from server? -> maybe server pings to see if client still alive?
            struct icmp icmp_packet =
                {
                    .username = "userTest", // receive from user from argv or connect request
                    .roomname = "room",
                    //! test more than 20 chars here - overflow!
                    // .message = "message im the best! How are you?",
                    .icmp_type = ICMP_ECHO
                };

            //? icmp.message is unsigned char so i get warning between char and unsigned char
            strncpy(icmp_packet.message, msg_buffer, 20);

            if (sendto(sockfd, &icmp_packet, sizeof(icmp_packet), 0, (struct sockaddr *)&dest_addr, sizeof(dest_addr)) < 0)
            {
                perror("sendto");
                close(sockfd);
                exit(EXIT_FAILURE);
            }
        }

        // Socket has data! recv() it and printf().
        if (fds[1].revents & POLLIN) {
            //! receive back from server -> doesnt work for now, we are catching our own echo the one that the kernel sends back not the one sent by our server
            char buffer[1000] = {0};
            ssize_t bytes_received = recvfrom(sockfd, buffer, sizeof(buffer), 0, NULL, NULL);
            // printf("Received %zd bytes.\n", bytes_received);
            struct icmp *icmp_reply = (struct icmp *)(buffer); // Skip IP header

            //? is bytes_received > 0 protection necessary
            if (bytes_received > 0 && icmp_reply->icmp_type == ICMP_ECHO_REPLY)
            {
                // printf("Received %zd bytes inside the echo REPLY\n", bytes_received);

                // printf("IP Header:\n");
                // printf("  Version: %u\n", ip_header->version);
                // printf("  Header Length: %u\n", ip_header->ihl * 4);
                // printf("  Source IP: %s\n", inet_ntoa(*(struct in_addr *)&ip_header->saddr));
                // printf("  Destination IP: %s\n", inet_ntoa(*(struct in_addr *)&ip_header->daddr));
                // printf("  Protocol: %u\n", ip_header->protocol);

                // printf("ICMP data:\n");
                // printf("  Type: %u\n", icmp_header->icmp_type);
                // printf("  Code: %u\n", icmp_header->icmp_code);
                // printf("%s: ", icmp_reply->username);
                // printf("  %s\n", icmp_reply->roomname);
                // printf("%s\n", icmp_reply->message);

                printf("%s: %s\n", icmp_reply->username, icmp_reply->username);
            }
        }
    }

    close(sockfd);
    return 0;
}