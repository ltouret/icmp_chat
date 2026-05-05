#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
// #include <netinet/ip_icmp.h>
#include <netinet/ip.h>
#include <arpa/inet.h>
#include <poll.h>

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
    //? usernames dont seem necessary, each time and user sends a msg if broadcast directly his username to all the others
    // ? but all the others receive the username so keeping those in memory is useless no?
    char users[100][11]; //! maybe transform users to ints? so i just keep 100 ints here? -> user sam logs in becomes 1, etc
    uint32_t ips[100]; // save the ips of everyone in the room -> need to add protection for this
    unsigned char user_counter;
    //? add struct of users her
    struct room *next;
};

//? remember the order of typed elements is important in c structures maybe adapt this
// struct user
// {
//     char username[11];
//     uint64_t last_seen;
//     struct user *next;
// }

//? careful with the use of strncpy as it is not safe this can overflow!
void create_room(struct room **room, const char *roomname)
{
    if (*room == NULL)
    {
        // malloc
        *room = calloc(1, sizeof(struct room));
        if (*room == NULL) {
            perror("Failed to allocate memory for room");
            exit(EXIT_FAILURE);
        }
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
        if (strcmp(current->roomname, roomname) == 0)
        {
            return;
        }
        current = current->next;
    }

    // printf("current is %s\n", current->roomname);
    // else create
    // malloc and add to linked list
    current->next = calloc(1, sizeof(struct room));
    if (current->next == NULL) {
        // Handle error: exit or return
        perror("Failed to allocate memory for new room");
        exit(EXIT_FAILURE);
    }
    // current->next-> = malloc(sizeof(struct room));
    //! if roomname more than size 10 then this overflows! addd protection
    strcpy(current->next->roomname, roomname);
    current->next->next = NULL;
    // printf("current after %s\n", current->next->roomname);
}

// helper debug function remove after
void print_ip(uint32_t ip) {
    struct in_addr ip_addr;
    ip_addr.s_addr = ip;
    printf("IP Address: %s\n", inet_ntoa(ip_addr));
}

//? for debugging maybe send username as parameter for now to show if it works correctly
// add_if_not_in_room(icmp_header, room, ip_header->saddr);
void add_if_not_in_room(char *roomname, struct room *room, uint32_t ip)
{
    /* This function is supposed to: 
        - check if user is not in room already
            - if exists we do nothing
            - if doesnt exists we add to room linked list and add ip to room ips
        - if roomname doesnt exist do nothing
    */
    struct room *current = room;

    while (current)
    {
        if (strcmp(current->roomname, roomname) == 0)
        {
            break;
        }
        current = current->next;
    }
    if (current == NULL)
    {
        printf("Roomname doesnt exist so we dont anything\n");
        return;
    }
    //! this if is not necessary now then
    for (int i = 0; i < 100; i++)
    {
        if (current->ips[i] == ip)
        {
            printf("Ip already in room: ");
            print_ip(ip);
            return;
        }
    }
    const unsigned char user_counter = current->user_counter;
    if (user_counter >= 100)
    {
        printf("Room is full, cannot add more users\n");
        return;
    }
    current->ips[user_counter] = ip;
    current->user_counter++;
    //? for debugging maybe send username as parameter for now to show if it works correctly
    printf("Added to roomname %s the ip: ", roomname);
    print_ip(ip);
}

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

int main()
{
    int sockfd;
    struct room *room = NULL;
    //? free this at the end of the program, but since its infinite loop it shouldnt be a problem for now
    create_room(&room, "room");
    unsigned char buffer[1000] = {0};

    sockfd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
    if (sockfd < 0)
    {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    struct pollfd fd;
    fd.fd = sockfd;     // Watch socket
    fd.events = POLLIN;

    while (1)
    {
        //! code other part that will just reply to pings maybe?
        //! maybe buffer should be re init to 0 after each recvfrom? to avoid parsing old data if new packet is smaller than old one
        //! buffer size is a question too using 1000 for testing purposes

        //! ping here before everything
        // - send_all_pings() - every 5? seconds
	    // - cleanup() When do i run this? - If users Timestamp more than 10 seconds then remove from room.

        //? here calculate timeout for poll to be able to send pings to clients to check if they are still alive,
        // 
        int ret = poll(&fd, 1, 2000);

        if (ret > 0) {
            if (fd.revents & POLLIN) {
                //? recvfrom
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
                    //! change the name of this or use it only to get the header not all the data!
                    struct icmp *icmp_header = (struct icmp *)(buffer + ip_header->ihl * 4); // Skip IP header

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
                        printf("  %d\n", icmp_header->icmp_id); //? for the ID we need maybe to keep track of this and keep augmenting it as a normal ping
                        printf("  %s\n", icmp_header->message);

                        //! add packet validation to see if it comes from a client, maybe create a checksum function? -> could add it to icmp_header->padding, 
                        //! like that i know when its a ping of mine, and for all else i can do a normal ping reply
                        // ? for now only one room so this must be skipped
                        // ? best way to do this is to hardcode room logic to name "room" so users cant change of room,
                        // ? so we ignore the value received from the user
                        if (!strcmp(icmp_header->roomname, "room"))
                            printf("same room, validation ok\n");

                        //? here we add user if not already in room 
                        //! reply back to the clients, infinite loop for now bc we reply and then our recv receives it as we are using the same ip address for server and client
                        //? this should be done in the for loop where we send the packet to all users in the room.
                        // init to zero this!
                        struct sockaddr_in dest_addr = {0};
                        dest_addr.sin_family = AF_INET;
                        dest_addr.sin_addr.s_addr = ip_header->saddr; //! TODO TEST THIS! instead of if
                        // if (inet_pton(AF_INET, inet_ntoa(*(struct in_addr *)&ip_header->saddr), &dest_addr.sin_addr) <= 0)
                        // {
                        //     perror("inet_pton");
                        //     close(sockfd);
                        //     exit(EXIT_FAILURE);
                        // }

                        add_if_not_in_room(icmp_header->roomname, room, ip_header->saddr);

                        //? here we create the broadcast icmp packet
                        struct icmp icmp_packet =
                        {
                            // .username = icmp_header->username, // <- get username from the icmp packet received from the client, so we can broadcast to all the users in the room who sent the message
                            .roomname = "room", //? hardcoded as we for now only have one room, this feature will arrive later
                            // .message = icmp_header->message, //? <- here i add the message that i want to broadcast to all the users in the room, for now its just a test message but later it will be the message received from the user
                            .icmp_id = icmp_header->icmp_id,
                            .icmp_type = ICMP_ECHO_REPLY,
                            // .icmp_cksum = 0, //? we will calculate the checksum later, after we fill the packet, and then we will add it to the packet, so we need to set it to 0 for now to calculate the checksum correctly
                        };

                        //! no magic number add a define with max size of msgs and logic to cut messages if too big maybe?
                        strncpy(icmp_packet.message, icmp_header->message, 20);
                        strncpy(icmp_packet.username, icmp_header->username, 10);

                        icmp_packet.icmp_cksum = 0;
                        icmp_packet.icmp_cksum = calculate_checksum((unsigned short *)&icmp_packet, sizeof(icmp_packet));
                        //? here we send the icmp packet to the whole linked list of users by iterating through all the rooms.ips
                        for (int i = 0; i < room->user_counter; i++) {
                            //? add ip to dest_addr then send it!
                            dest_addr.sin_addr.s_addr = room->ips[i]; //! TODO TEST THIS! instead of if
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
            }
        }
    }

    close(sockfd);
    return 0;
}