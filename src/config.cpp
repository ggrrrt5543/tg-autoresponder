#include "config.h"
#include "json.hpp"
#include <fstream>
#include <iostream>

using json = nlohmann::json;

ConfigManager& ConfigManager::instance() {
    static ConfigManager inst;
    return inst;
}

Config ConfigManager::get() const {
    std::lock_guard<std::mutex> lock(mtx_);
    return config_;
}

void ConfigManager::set_active(bool active) {
    std::lock_guard<std::mutex> lock(mtx_);
    config_.bot_active = active;
    save();
}

void ConfigManager::add_chat(int64_t chat_id) {
    std::lock_guard<std::mutex> lock(mtx_);
    for (auto id : config_.monitored_chats)
        if (id == chat_id) return;
    config_.monitored_chats.push_back(chat_id);
    save();
}

void ConfigManager::clear_chats() {
    std::lock_guard<std::mutex> lock(mtx_);
    config_.monitored_chats.clear();
    save();
}

void ConfigManager::set_answer(const std::string& text) {
    std::lock_guard<std::mutex> lock(mtx_);
    config_.answer_text = text;
    save();
}

void ConfigManager::set_limits(double min_val, double max_val) {
    std::lock_guard<std::mutex> lock(mtx_);
    config_.limit_min = min_val;
    config_.limit_max = max_val;
    save();
}

void ConfigManager::load() {
    std::lock_guard<std::mutex> lock(mtx_);
    std::ifstream f(filepath_);
    if (!f.is_open()) {
        std::cout << "[config] config.json not found, using defaults.\n";
        return;
    }
    try {
        json j = json::parse(f);
        config_.bot_active = j.value("bot_active", false);
        config_.answer_text = j.value("answer_text", "");
        config_.limit_min = j.value("limit_min", 0.0);
        config_.limit_max = j.value("limit_max", 0.0);
        config_.monitored_chats.clear();
        if (j.contains("monitored_chats")) {
            for (auto& v : j["monitored_chats"])
                config_.monitored_chats.push_back(v.get<int64_t>());
        }
        std::cout << "[config] Loaded.\n";
    } catch (const std::exception& e) {
        std::cerr << "[config] Parse error: " << e.what() << "\n";
    }
}

void ConfigManager::save() {
    // NOTE: called with lock already held
    json j;
    j["bot_active"] = config_.bot_active;
    j["answer_text"] = config_.answer_text;
    j["limit_min"] = config_.limit_min;
    j["limit_max"] = config_.limit_max;
    j["monitored_chats"] = config_.monitored_chats;

    std::ofstream f(filepath_);
    if (f.is_open()) {
        f << j.dump(2) << std::endl;
    }
}
