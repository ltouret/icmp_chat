#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip_icmp.h>
#include <arpa/inet.h>

int main(int argc, char *argv[])
{
    int sockfd;

    sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_ICMP);
    if (sockfd < 0)
    {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    printf("ICMP socket created successfully.\n");

    close(sockfd);
    return 0;
}