#ifndef SHARED_ICMP_H
#define SHARED_ICMP_H

#include <stdint.h>

// --- PROTOCOL CONSTANTS ---
//? magic numbers here!
#define PACKET_USERNAME_LEN 10
#define PACKET_ROOMNAME_LEN 10
#define PACKET_MESSAGE_LEN  20

#define ICMP_ECHO 8
#define ICMP_ECHO_REPLY 0

struct icmp
{
    // header 20 bytes
    uint8_t icmp_type;
    uint8_t icmp_code;
    uint16_t icmp_cksum;
    uint16_t icmp_id; // expect Big-Endian (Network Byte Order). htons
    uint16_t icmp_seq; // expect Big-Endian (Network Byte Order). htons

    //? this padding could be used for more data or some nounce for example (could be fun :D)
    uint32_t icmp_otime; // padding
    uint32_t icmp_rtime; // padding
    uint32_t icmp_ttime; // padding

    // padding 4 bytes
    uint32_t padding;

    // data 40 bytes
    // uint8_t data[40];
    uint8_t username[10];
    uint8_t roomname[10];
    //! try with more len for message
    uint8_t message[20];
};// __attribute__((packed)); //! <-- fix the warning!

#endif // SHARED_ICMP_H