#pragma once
#include <string>

struct PingResult {
    bool success;
    double rtt_ms;
    std::string ip;
    int ttl;
};

PingResult send_ping(const std::string& host, int sequence, int timeout_sec = 2);