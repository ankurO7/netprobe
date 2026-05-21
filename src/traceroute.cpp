#include "traceroute.h"
#include <iostream>
#include <cstring>
#include <sys/socket.h>
#include <netinet/ip_icmp.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/time.h>
#include <unistd.h>

uint16_t chksum(void* b, int len) {
    uint16_t* buf = (uint16_t*)b;
    uint32_t sum = 0;
    for (; len > 1; len -= 2) sum += *buf++;
    if (len == 1) sum += *(uint8_t*)buf;
    sum = (sum >> 16) + (sum & 0xFFFF);
    sum += (sum >> 16);
    return ~sum;
}

// Reverse DNS — turns IP into hostname
std::string resolve_hostname(const std::string& ip) {
    struct sockaddr_in sa;
    sa.sin_family = AF_INET;
    inet_pton(AF_INET, ip.c_str(), &sa.sin_addr);
    char host[256];
    if (getnameinfo((struct sockaddr*)&sa, sizeof(sa), host, sizeof(host), nullptr, 0, 0) == 0)
        return std::string(host);
    return ip; // fallback to IP if no hostname
}

void run_traceroute(const std::string& host, int max_hops) {
    struct hostent* he = gethostbyname(host.c_str());
    if (!he) { std::cerr << "Could not resolve host\n"; return; }

    struct sockaddr_in dest;
    memset(&dest, 0, sizeof(dest));
    dest.sin_family = AF_INET;
    dest.sin_addr = *(struct in_addr*)he->h_addr;
    std::string dest_ip = inet_ntoa(dest.sin_addr);

    std::cout << "Tracing route to " << host << " (" << dest_ip << ")\n";
    std::cout << "Max hops: " << max_hops << "\n\n";
    printf("%-5s %-20s %-35s %s\n", "Hop", "IP", "Hostname", "RTT");
    std::cout << std::string(75, '-') << "\n";

    int sock = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
    if (sock < 0) { std::cerr << "Error: Run with sudo\n"; return; }

    struct timeval tv = {2, 0};
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

    for (int ttl = 1; ttl <= max_hops; ttl++) {
        // Set TTL — this is the traceroute trick
        setsockopt(sock, IPPROTO_IP, IP_TTL, &ttl, sizeof(ttl));

        uint8_t packet[64];
        memset(packet, 0, sizeof(packet));
        struct icmphdr* icmp = (struct icmphdr*)packet;
        icmp->type = ICMP_ECHO;
        icmp->code = 0;
        icmp->un.echo.id = getpid();
        icmp->un.echo.sequence = ttl;
        icmp->checksum = chksum(packet, sizeof(packet));

        struct timeval start, end;
        gettimeofday(&start, nullptr);
        sendto(sock, packet, sizeof(packet), 0, (struct sockaddr*)&dest, sizeof(dest));

        uint8_t recv_buf[1024];
        struct sockaddr_in recv_addr;
        socklen_t addr_len = sizeof(recv_addr);

        if (recvfrom(sock, recv_buf, sizeof(recv_buf), 0, (struct sockaddr*)&recv_addr, &addr_len) > 0) {
            gettimeofday(&end, nullptr);
            double rtt = (end.tv_sec - start.tv_sec) * 1000.0
                       + (end.tv_usec - start.tv_usec) / 1000.0;

            std::string hop_ip = inet_ntoa(recv_addr.sin_addr);
            std::string hostname = resolve_hostname(hop_ip);

            printf("%-5d %-20s %-35s %.2f ms\n", ttl, hop_ip.c_str(), hostname.c_str(), rtt);

            // Stop if we've reached the destination
            if (hop_ip == dest_ip) {
                std::cout << "\nReached destination!\n";
                break;
            }
        } else {
            printf("%-5d %-20s\n", ttl, "* * * (timeout)");
        }
    }

    close(sock);
}