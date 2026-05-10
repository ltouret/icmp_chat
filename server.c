#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
// #include <netinet/ip_icmp.h>
#include <netinet/ip.h>
#include <arpa/inet.h>
#include <poll.h>
#include <time.h>

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
    uint32_t ips[100]; // save the ips of everyone in the room -> need to add protection for this //? use a define not a magic number
    unsigned char user_counter;
    struct user *user_list_head;
    struct user *user_list_tail;
    //? add struct of users her
    struct room *next;
};

//? remember the order of typed elements is important in c structures maybe adapt this
struct user
{
    char username[11];
    uint32_t ip;
    uint64_t last_seen;
    struct user *next;
};

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

//! check this func?
// int64_t currentTimeMillis() {
//     struct timeval time;
//     gettimeofday(&time, NULL);
//     int64_t s1 = (int64_t)(time.tv_sec) * 1000;
//     int64_t s2 = (time.tv_usec / 1000);
//     return s1 + s2;
// }

int64_t getMonotonicMillis() {
    struct timespec ts;
    // CLOCK_MONOTONIC is immune to system time changes
    clock_gettime(CLOCK_MONOTONIC, &ts);
    // int64_t ret = (int64_t)ts.tv_sec * 1000 + (ts.tv_nsec / 1000000);
    // printf("getMonotonicMillis %lld ", ret);
    // return ret;
    return (int64_t)ts.tv_sec * 1000 + (ts.tv_nsec / 1000000);
}

struct user *create_user_in_room(char *username, uint32_t ip)
{
    struct user *new_user = calloc(1, sizeof(struct user));
    if (!new_user)
    {
        perror("Failed to allocate memory for user");
        return NULL;
    }
    strcpy(new_user->username, username);
    new_user->last_seen = getMonotonicMillis();
    new_user->ip = ip;
    new_user->next = NULL;
}

//? for debugging maybe send username as parameter for now to show if it works correctly
// add_if_not_in_room(icmp_header, room, ip_header->saddr);
void add_if_not_in_room(char *roomname, char *username, struct room *room, uint32_t ip)
{
    /* This function is supposed to: 
        - check if user is not in room already
            - if exists we do nothing
            - if doesnt exists we add to room linked list and add ip to room ips
        - if roomname doesnt exist do nothing
    */
    struct room *current_room = room;

    while (current_room)
    {
        if (strcmp(current_room->roomname, roomname) == 0)
        {
            break;
        }
        current_room = current_room->next;
    }

    if (current_room == NULL)
    {
        printf("Roomname doesnt exist so we dont do anything\n");
        return;
    }
    
    //TODO change magic number here and maybe max clients 1k?
    const unsigned char user_counter = current_room->user_counter;
    if (user_counter >= 100)
    {
        printf("Room is full, cannot add more users\n");
        return;
    }

    // TODO
    //! this will go into its own function to add ip to room
    //? this for now checks only if user ip is in room -> change it!
    //? we need to check if user ip + username + some kind of unique id is in room
    struct user *current_user = current_room->user_list_head;

    //TODO
    //? this is if adding first user
    if (!current_user)
    {
        //TODO
        //! this goes into its own function as appending a new user is always the same!
        struct user *new_user = create_user_in_room(username, ip);
        if (!new_user)
        {
            perror("Failed to allocate memory for user");
            exit(EXIT_FAILURE);
        }

        current_room->user_list_head = new_user;
        current_room->user_list_tail = new_user;

        current_room->user_counter++;

        printf("Added to roomname %s the at time: %lld to the ", roomname, new_user->last_seen);
        print_ip(ip);
        return;
    }

    while (current_user)
    {
        if (current_user->ip == ip)
        {
            printf("Ip already in room: ");
            print_ip(ip);
            return;
        }
        current_user = current_user->next;
    }

    // TODO
    //? as then current_user == NULL so i can't add the new user to the tail but as i dont need to add it in order
    //? then its better to add it to the head, with the last_seen = now, like that the fist element is always the new one?
    //? or we can try to add it to the tail and have some kind of order in which the head are the "stale" or old client 
    //? that should be removed!

    //? this is if adding second - third users ++
    //! this goes into its own function as appending a new user is always the same!
    struct user *new_user = create_user_in_room(username, ip);
    if (!new_user)
    {
        perror("Failed to allocate memory for user");
        exit(EXIT_FAILURE);
    }

    current_room->user_list_tail->next = new_user;
    current_room->user_list_tail = new_user;

    current_room->user_counter++;
    printf("Added to roomname %s the at time: %lld to the ", roomname, new_user->last_seen);
    print_ip(ip);

    return;
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
    fd.fd = sockfd;
    fd.events = POLLIN;

    //? better than just 1 and can be used later for control c the program
    char running = 1;
    while (running)
    {
        //! code other part that will just reply to pings maybe?
        //! maybe buffer should be re init to 0 after each recvfrom? to avoid parsing old data if new packet is smaller than old one
        //! buffer size is a question too using 1000 for testing purposes

        //! ping here before everything
        // - send_all_pings() - every 5? seconds
            /*  I think the Ping feature will have an error as the server will send one ping to the client,
                but the client kernel will reply to that ping and the server will receive that pong instead of the client pong
                that breaks my server ping - client pong logic, need to find a way to make it work.
                ideas for now:
                    - Custom ICMP packet that maybe the kernel wont answer so only the client handles? Might work or its going to be filtered by the kernel <- using this logic even the server wouldnt need to disable ping from the kernel 
                    - disable ping for client too (not great) 
            */
	    // - cleanup() When do i run this? - If users last_seen more than 10 seconds then remove from room.

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
                        {
                            // printf("same room, validation ok\n");
                        }

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

                        //? for now we just add to roomname "room" this will change when we add the different rooms
                        add_if_not_in_room(icmp_header->roomname, icmp_header->username, room, ip_header->saddr);

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

                        struct user *current_user = room->user_list_head;

                        while (current_user)
                        {
                            dest_addr.sin_addr.s_addr = current_user->ip;
                            if (sendto(sockfd, &icmp_packet, sizeof(icmp_packet), 0, (struct sockaddr *)&dest_addr, sizeof(dest_addr)) < 0)
                            {
                                perror("sendto");
                                close(sockfd);
                                exit(EXIT_FAILURE);
                            }
                            printf("sent\n");
                            current_user = current_user->next;
                        }
                    }
                }
            }
        }
    }

    close(sockfd);
    return 0;
}