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

#include "shared.h"

// #define ICMP_STRUCT_SIZE sizeof(struct icmp)
//? For dev only remove later
#define USERNAME "userTest"
#define ROOMNAME "room"

//! this is shared with server.c
unsigned short calculate_checksum(unsigned short *ptr, int nbytes)
{
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

//? this for now just sends a PING to server
// this means to every IP so if two clients have the same ip it will sent to that ip twice
// when I rework the unique id of an user ill change this too most likely
// ! if we havent sent any msgs what will happen here think about it!
    /*
    TODO think about how to have a unique user that the server will filter
        for now we send the ping to the server but we need a way for the server to read the
        icmp packet and get the userID if you will, to be able to change user->last_seen and reply back with the PONG packet

    TODO for now just send a special PING packet 
        What could a PING packet be?
    */
void send_ping(int sockfd, const struct sockaddr_in *dest_addr)
{
    // printf("called send_ping!\n");
    struct icmp icmp_packet =
    {
        //! for now ping will change later to real username
        .username = "PING", //! This is not longer needed to be "PING" if we add .action == ACTION_PING
        .roomname = ROOMNAME, //? hardcoded as we for now only have one room, this feature will arrive later
        .message = "PING", //? <- here i add the message that i want to broadcast to all the users in the room, for now its just a test message but later it will be the message received from the user
        // .icmp_id = 0,
        .icmp_type = ICMP_ECHO, // <- ICMP_ECHO as server sends to client
        .action = ACTION_PING
        //! use padding as some kind of actions. "JOIN" / "LEAVE" / "PING" / "MSG"
    };

    //! no magic number add a define with max size of msgs and logic to cut messages if too big maybe?
    // strncpy(icmp_packet.message, icmp_header->message, 20);
    // strncpy(icmp_packet.username, icmp_header->username, 10);

    icmp_packet.icmp_cksum = 0;
    //TODO change this to use the ICMP_STRUCT_SIZE define
    icmp_packet.icmp_cksum = calculate_checksum((unsigned short *)&icmp_packet, sizeof(icmp_packet));

    if (sendto(sockfd, &icmp_packet, sizeof(icmp_packet), 0, (struct sockaddr *)dest_addr, sizeof(*dest_addr)) < 0)
    {
        perror("send_ping sendto");
        close(sockfd);
        exit(EXIT_FAILURE);
    }
}

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
    struct sockaddr_in dest_addr = {0};

    sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_ICMP);
    if (sockfd < 0)
    {
        perror("socket init");
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

    //? better than just 1 and can be used later for control c the program
    char running = 1;
    while (running)
    {
        /*
            Every 5 seconds send a ping
                check >= 5 seconds passed from last time we sent_ping
                    if true -> send_ping and update time we wait in poll
                    else we calculate how much does poll have to (max) wait before we try again to send_ping
                poll will wait max x seconds dinamically, being x the how much time we need to wait until we send_ping again
                    this can be 5 seconds if we dont sendto or recv a msg from the server
                    or the seconds we need to wait until the total 5 seconds passed
                    for example if we waited 3 seconds and sento/recv a msg then next iteration poll will wait 2 seconds max (if we dont sendto/recv another msg) 

            TODO We need to recvmfrom a PONG from the server and save some kind of last_seen
                if 10 secs > last_seen
                    then disconnect from server?
        */
        // if (now() + 5 seconds >= time_that_should_send_ping())
            send_ping(sockfd, &dest_addr);
        poll(fds, 2, 5000); // wait max 5 seconds -> this will be dynamic

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
                    .username = USERNAME,  // receive from user from argv or connect request
                    .roomname = ROOMNAME,
                    //! test more than 20 chars here - overflow!
                    // .message = "message im the best! How are you?",
                    .icmp_type = ICMP_ECHO,
                    .action = ACTION_MSG
                };

            //? icmp.message is unsigned char so i get warning between char and unsigned char
            //? client.c:117:21: warning: passing 'uint8_t[20]' (aka 'unsigned char[20]') to parameter of type 'char *' converts between pointers to integer types where one is of the
            strncpy(icmp_packet.message, msg_buffer, 20);

            if (sendto(sockfd, &icmp_packet, sizeof(icmp_packet), 0, (struct sockaddr *)&dest_addr, sizeof(dest_addr)) < 0)
            {
                perror("sendto");
                close(sockfd);
                exit(EXIT_FAILURE);
            }
        }

        //? Socket has msg data! recv() it and printf().
        if (fds[1].revents & POLLIN) {
            //! receive back from server -> doesnt work for now, we are catching our own echo the one that the kernel sends back not the one sent by our server
            // TODO move buffer outside the while loop to not waste cpu re initing to 0
            char buffer[1001] = {0};
            //! const size_t expected_size = sizeof(struct icmp); // <- this should a DEFINE with the size of the icmp struct
            //TODO no magic numbers change buffer received size!
            ssize_t bytes_received = recvfrom(sockfd, buffer, 1000, 0, NULL, NULL);

            if (bytes_received < sizeof(struct icmp))
            {
                perror("recvfrom Packet too small");
                continue; // Or handle the error as needed
            }

            printf("Received %zd bytes.\n", bytes_received);
            struct icmp *icmp_reply = (struct icmp *)(buffer);

            //TODO change size here to use the ICMP_STRUCT_SIZE define
            // else if (icmp_header->icmp_type == ICMP_ECHO && !strcmp(icmp_header->username, "PING"))
            // in some cases we can receive back our own PING and treat it as a msg, this happens for example when
            // we ping something that is not our icmp server <- check!
            //! else if (icmp_header->icmp_type == ICMP_ECHO && !strcmp(icmp_header->username, "PING") && icmp_header->action == 1)
            // TODO test this **
            if (icmp_reply->icmp_type == ICMP_ECHO_REPLY)
            {
                // ! remove icmp_reply->icmp_type == ICMP_ECHO_REPLY into its own like that we check it once
                if (icmp_reply->icmp_type == ICMP_ECHO_REPLY && icmp_reply->action == ACTION_MSG)
                {
                    // Protection to not read data from a past message will be useful when we stop re zeroing the buffer every message
                    buffer[bytes_received] = '\0';

                    //TODO here we check if its really a MSG if it is print to client screen
                    //? if ICMP_ECHO_REPLY but we dont have any other protection then any packet as this will be shown to the client
                    //? any protection i can add?

                    //! this limits size of username and message to 10 and 20 chars, maybe add some kind of buffer split for bigger messages?
                    printf("%d id: %.10s: %.20s\n", icmp_reply->icmp_id, icmp_reply->username, icmp_reply->message);
                }

                //? Socket has PONG data - received from the server with a ICMP_ECHO_REPLY
                // TODO if its a PONG from the server reply PONG we keep sending msg, if we dont receive it then we disconnect?
                //? if else?
                // ! remove icmp_reply->icmp_type == ICMP_ECHO_REPLY into its own like that we check it once
                else if (icmp_reply->icmp_type == ICMP_ECHO_REPLY && icmp_reply->action == ACTION_PONG)
                {
                    //TODO here we check if its really a PING if it is reply PONG (ICMP_ECHO_REPLY)
                    // - For now the server wont send back something to the client so it knows the server still alive? <- Yes but later and what
                    printf("Received PONG packet from the server\n");
                }
            }
        }
    }

    close(sockfd);
    return 0;
}