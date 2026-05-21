#include <iostream>
#include <string>
#include <unistd.h>
#include "ping.h"
#include "traceroute.h"

void print_usage() {
    std::cout << "\nNetProbe — Network Diagnostic Tool\n";
    std::cout << "Usage:\n";
    std::cout << "  sudo ./netprobe ping <host> [count]\n";
    std::cout << "  sudo ./netprobe trace <host>\n\n";
    std::cout << "Examples:\n";
    std::cout << "  sudo ./netprobe ping google.com\n";
    std::cout << "  sudo ./netprobe ping 8.8.8.8 10\n";
    std::cout << "  sudo ./netprobe trace google.com\n\n";
}

int main(int argc, char* argv[]) {
    if (argc < 3) { print_usage(); return 1; }

    std::string command = argv[1];
    std::string host = argv[2];

    if (command == "ping") {
        int count = (argc >= 4) ? std::stoi(argv[3]) : 4;

        std::cout << "\nPinging " << host << " with " << count << " packets:\n\n";
        printf("%-6s %-20s %-10s %s\n", "Seq", "IP", "TTL", "RTT");
        std::cout << std::string(50, '-') << "\n";

        int success = 0;
        double total_rtt = 0;

        for (int i = 1; i <= count; i++) {
            PingResult r = send_ping(host, i);
            if (r.success) {
                printf("%-6d %-20s %-10d %.2f ms\n", i, r.ip.c_str(), r.ttl, r.rtt_ms);
                success++;
                total_rtt += r.rtt_ms;
            } else {
                printf("%-6d %-20s timeout\n", i, host.c_str());
            }
            sleep(1);
        }

        std::cout << std::string(50, '-') << "\n";
        std::cout << success << "/" << count << " packets received";
        if (success > 0)
            printf(", avg RTT: %.2f ms\n\n", total_rtt / success);
        else
            std::cout << "\n\n";

    } else if (command == "trace") {
        run_traceroute(host);
    } else {
        print_usage();
        return 1;
    }

    return 0;
}