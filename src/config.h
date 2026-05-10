#pragma once

#include <string>
#include <vector>
#include <mutex>

struct Config {
    bool bot_active = false;
    std::vector<int64_t> monitored_chats;  // channels/chats to watch for orders
    int64_t reply_chat = 0;                // chat where to send reply (0 = same chat)
    std::string answer_text;
    double limit_min = 0.0;
    double limit_max = 0.0;
};

class ConfigManager {
public:
    static ConfigManager& instance();

    void load();
    void save();

    Config get() const;
    void set_active(bool active);
    void add_chat(int64_t chat_id);
    void clear_chats();
    void set_reply_chat(int64_t chat_id);
    void set_answer(const std::string& text);
    void set_limits(double min_val, double max_val);

private:
    ConfigManager() = default;
    mutable std::mutex mtx_;
    Config config_;
    std::string filepath_ = "config.json";
};
