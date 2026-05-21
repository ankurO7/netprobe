#pragma once
#include <string>

struct HopResult {
    int hop;
    std::string ip;
    std::string hostname;
    double rtt_ms;
    bool reached_dest;
    bool timed_out;
};

void run_traceroute(const std::string& host, int max_hops = 30);