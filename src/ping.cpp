#include "ping.h"
#include <iostream>
#include <cstring>
#include <sys/socket.h>
#include <netinet/ip_icmp.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/time.h>
#include <unistd.h>

// Calculates ICMP checksum — required for packet to be valid
uint16_t checksum(void* b, int len) {
    uint16_t* buf = (uint16_t*)b;
    uint32_t sum = 0;
    for (; len > 1; len -= 2) sum += *buf++;
    if (len == 1) sum += *(uint8_t*)buf;
    sum = (sum >> 16) + (sum & 0xFFFF);
    sum += (sum >> 16);
    return ~sum;
}

PingResult send_ping(const std::string& host, int sequence, int timeout_sec) {
    PingResult result = {false, 0.0, "", 0};

    // Resolve hostname to IP
    struct hostent* he = gethostbyname(host.c_str());
    if (!he) { return result; }

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr = *(struct in_addr*)he->h_addr;
    result.ip = inet_ntoa(addr.sin_addr);

    // Open raw socket — requires sudo
    int sock = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
    if (sock < 0) {
        std::cerr << "Error: Run with sudo\n";
        return result;
    }

    // Set receive timeout
    struct timeval tv = {timeout_sec, 0};
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

    // Build ICMP echo request packet
    uint8_t packet[64];
    memset(packet, 0, sizeof(packet));
    struct icmphdr* icmp = (struct icmphdr*)packet;
    icmp->type = ICMP_ECHO;
    icmp->code = 0;
    icmp->un.echo.id = getpid();
    icmp->un.echo.sequence = sequence;
    icmp->checksum = checksum(packet, sizeof(packet));

    // Record send time
    struct timeval start, end;
    gettimeofday(&start, nullptr);

    // Send packet
    if (sendto(sock, packet, sizeof(packet), 0, (struct sockaddr*)&addr, sizeof(addr)) <= 0) {
        close(sock);
        return result;
    }

    // Wait for reply
    uint8_t recv_buf[1024];
    struct sockaddr_in recv_addr;
    socklen_t addr_len = sizeof(recv_addr);

    if (recvfrom(sock, recv_buf, sizeof(recv_buf), 0, (struct sockaddr*)&recv_addr, &addr_len) > 0) {
        gettimeofday(&end, nullptr);

        result.rtt_ms = (end.tv_sec - start.tv_sec) * 1000.0
                      + (end.tv_usec - start.tv_usec) / 1000.0;

        struct iphdr* ip = (struct iphdr*)recv_buf;
        result.ttl = ip->ttl;
        result.success = true;
    }

    close(sock);
    return result;
}