#pragma once
#include <string>
#include <functional>
#include <cstdint>

// send(chat_id, message_id_to_reply, text)
using SendFn = std::function<void(int64_t chat_id, int64_t reply_to_msg_id, const std::string& text)>;

// Performance stats
struct PerfStats {
    int64_t last;
    int64_t best;
    double avg;
    int64_t count;
};

PerfStats get_perf_stats();
int64_t get_last_ping_ms();

// Returns true if message was handled
bool handle_message(int64_t chat_id, int64_t msg_id, int64_t sender_id, int64_t my_id,
                    const std::string& text, const SendFn& send);
