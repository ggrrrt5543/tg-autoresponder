#include "handler.h"
#include "config.h"
#include "utils.h"
#include <sstream>
#include <iostream>

static bool starts_with(const std::string& s, const std::string& p) {
    return s.size() >= p.size() && s.compare(0, p.size(), p) == 0;
}

bool handle_message(int64_t chat_id, int64_t msg_id, int64_t sender_id, int64_t my_id,
                    const std::string& text, const SendFn& send) {
    auto& cm = ConfigManager::instance();

    // Commands — check sender (log for debug)
    bool is_command = starts_with(text, "/");
    if (is_command) {
        std::cout << "[cmd] sender_id=" << sender_id << " my_id=" << my_id << " text=" << text << "\n";
    }

    // Accept commands from ourselves (or if my_id not yet known)
    if (is_command && (sender_id == my_id || my_id == 0)) {

        // /set — show all commands
        if (text == "/set") {
            std::string msg =
                "\xf0\x9f\x93\x8b \xd0\x9a\xd0\xbe\xd0\xbc\xd0\xb0\xd0\xbd\xd0\xb4\xd1\x8b:\n\n"
                "\xf0\x9f\x9f\xa2 /start_bot \xe2\x80\x94 \xd0\xb2\xd0\xba\xd0\xbb\xd1\x8e\xd1\x87\xd0\xb8\xd1\x82\xd1\x8c\n"
                "\xf0\x9f\x94\xb4 /stop_bot \xe2\x80\x94 \xd0\xb2\xd1\x8b\xd0\xba\xd0\xbb\xd1\x8e\xd1\x87\xd0\xb8\xd1\x82\xd1\x8c\n"
                "\xf0\x9f\x93\x8a /status \xe2\x80\x94 \xd1\x81\xd1\x82\xd0\xb0\xd1\x82\xd1\x83\xd1\x81\n"
                "\xf0\x9f\x92\xac /upchat <id> \xe2\x80\x94 \xd0\xb4\xd0\xbe\xd0\xb1\xd0\xb0\xd0\xb2\xd0\xb8\xd1\x82\xd1\x8c \xd1\x87\xd0\xb0\xd1\x82\n"
                "\xf0\x9f\x97\x91 /clear \xe2\x80\x94 \xd0\xbe\xd1\x87\xd0\xb8\xd1\x81\xd1\x82\xd0\xb8\xd1\x82\xd1\x8c \xd1\x87\xd0\xb0\xd1\x82\xd1\x8b\n"
                "\xe2\x9c\x8f\xef\xb8\x8f /ans_chat <\xd1\x82\xd0\xb5\xd0\xba\xd1\x81\xd1\x82> \xe2\x80\x94 \xd0\xbe\xd1\x82\xd0\xb2\xd0\xb5\xd1\x82\n"
                "\xf0\x9f\x93\x8f /limit <min> <max> \xe2\x80\x94 \xd0\xbb\xd0\xb8\xd0\xbc\xd0\xb8\xd1\x82\xd1\x8b\n"
                "\xf0\x9f\x93\x8b /set \xe2\x80\x94 \xd1\x8d\xd1\x82\xd0\xbe \xd0\xbc\xd0\xb5\xd0\xbd\xd1\x8e";
            send(chat_id, 0, msg);
            return true;
        }

        // /start_bot
        if (text == "/start_bot") {
            cm.set_active(true);
            send(chat_id, 0, "\xf0\x9f\x9f\xa2 \xd0\x90\xd0\xb2\xd1\x82\xd0\xbe\xd0\xbe\xd1\x82\xd0\xb2\xd0\xb5\xd1\x82\xd1\x87\xd0\xb8\xd0\xba \xd0\xb2\xd0\xba\xd0\xbb\xd1\x8e\xd1\x87\xd0\xb5\xd0\xbd");
            std::cout << "[bot] Activated\n";
            return true;
        }

        // /stop_bot
        if (text == "/stop_bot") {
            cm.set_active(false);
            send(chat_id, 0, "\xf0\x9f\x94\xb4 \xd0\x90\xd0\xb2\xd1\x82\xd0\xbe\xd0\xbe\xd1\x82\xd0\xb2\xd0\xb5\xd1\x82\xd1\x87\xd0\xb8\xd0\xba \xd0\xb2\xd1\x8b\xd0\xba\xd0\xbb\xd1\x8e\xd1\x87\xd0\xb5\xd0\xbd");
            std::cout << "[bot] Deactivated\n";
            return true;
        }

        // /clear — clear monitored chats
        if (text == "/clear") {
            auto cfg = cm.get();
            int count = (int)cfg.monitored_chats.size();
            cm.clear_chats();
            std::ostringstream o;
            o << "\xf0\x9f\x97\x91 \xd0\x9e\xd1\x87\xd0\xb8\xd1\x89\xd0\xb5\xd0\xbd\xd0\xbe " << count << " \xd1\x87\xd0\xb0\xd1\x82(\xd0\xbe\xd0\xb2)";
            send(chat_id, 0, o.str());
            std::cout << "[bot] Cleared " << count << " chats\n";
            return true;
        }

        // /status — beautiful status
        if (text == "/status") {
            auto cfg = cm.get();
            auto perf = get_perf_stats();
            std::ostringstream o;
            o << "\xe2\x9a\xa1 C++ AUTORESPONDER\n";
            o << "\xe2\x94\x81\xe2\x94\x81\xe2\x94\x81\xe2\x94\x81\xe2\x94\x81\xe2\x94\x81\xe2\x94\x81\xe2\x94\x81\xe2\x94\x81\xe2\x94\x81\xe2\x94\x81\xe2\x94\x81\xe2\x94\x81\xe2\x94\x81\xe2\x94\x81\xe2\x94\x81\xe2\x94\x81\xe2\x94\x81\xe2\x94\x81\xe2\x94\x81\n";

            // Active status
            if (cfg.bot_active)
                o << "\xf0\x9f\x9f\xa2 Status: RUNNING\n";
            else
                o << "\xf0\x9f\x94\xb4 Status: STOPPED\n";

            // Limits
            o << "\xf0\x9f\x94\xa7 Limits: " << cfg.limit_min << " \xe2\x80\x94 " << cfg.limit_max << "\n";

            // Answer
            o << "\xf0\x9f\x8e\xaf Mode: REPLY\n";

            // Performance
            o << "\xe2\x94\x81\xe2\x94\x81\xe2\x94\x81\xe2\x94\x81\xe2\x94\x81\xe2\x94\x81\xe2\x94\x81\xe2\x94\x81\xe2\x94\x81\xe2\x94\x81\xe2\x94\x81\xe2\x94\x81\xe2\x94\x81\xe2\x94\x81\xe2\x94\x81\xe2\x94\x81\xe2\x94\x81\xe2\x94\x81\xe2\x94\x81\xe2\x94\x81\n";
            o << "\xf0\x9f\x9a\x80 PERFORMANCE\n";
            if (perf.count > 0) {
                o << "  Last: " << perf.last << " ms\n";
                o.precision(2);
                o << std::fixed;
                o << "  Avg:  " << perf.avg << " ms\n";
                o << "  Best: " << perf.best << " ms\n";
                o << "  Hits: " << perf.count << "\n";
            } else {
                o << "  \xe2\x80\x94 no data yet\n";
            }

            // Answer text
            o << "\xe2\x94\x81\xe2\x94\x81\xe2\x94\x81\xe2\x94\x81\xe2\x94\x81\xe2\x94\x81\xe2\x94\x81\xe2\x94\x81\xe2\x94\x81\xe2\x94\x81\xe2\x94\x81\xe2\x94\x81\xe2\x94\x81\xe2\x94\x81\xe2\x94\x81\xe2\x94\x81\xe2\x94\x81\xe2\x94\x81\xe2\x94\x81\xe2\x94\x81\n";
            o << "\xe2\x9c\x8f\xef\xb8\x8f Answer: " << (cfg.answer_text.empty() ? "(none)" : cfg.answer_text) << "\n";

            // Chats
            o << "\xf0\x9f\x92\xac Chats (" << cfg.monitored_chats.size() << "):\n";
            if (cfg.monitored_chats.empty()) {
                o << "   \xe2\x80\xa2 none\n";
            } else {
                for (auto id : cfg.monitored_chats)
                    o << "   \xe2\x80\xa2 " << id << "\n";
            }

            send(chat_id, 0, o.str());
            return true;
        }

        // /upchat <id>
        if (starts_with(text, "/upchat ")) {
            try {
                int64_t id = std::stoll(text.substr(8));
                cm.add_chat(id);
                std::ostringstream o;
                o << "\xe2\x9c\x85 \xd0\xa7\xd0\xb0\xd1\x82 " << id << " \xd0\xb4\xd0\xbe\xd0\xb1\xd0\xb0\xd0\xb2\xd0\xbb\xd0\xb5\xd0\xbd";
                send(chat_id, 0, o.str());
                std::cout << "[bot] Added chat " << id << "\n";
            } catch (...) {
                send(chat_id, 0, "\xe2\x9d\x8c \xd0\x9d\xd0\xb5\xd0\xb2\xd0\xb5\xd1\x80\xd0\xbd\xd1\x8b\xd0\xb9 chat_id");
            }
            return true;
        }

        // /ans_chat <text>
        if (starts_with(text, "/ans_chat ")) {
            std::string ans = text.substr(10);
            cm.set_answer(ans);
            send(chat_id, 0, "\xe2\x9c\x85 \xd0\x9e\xd1\x82\xd0\xb2\xd0\xb5\xd1\x82: " + ans);
            std::cout << "[bot] Answer: " << ans << "\n";
            return true;
        }

        // /limit <min> <max>
        if (starts_with(text, "/limit ")) {
            std::istringstream iss(text.substr(7));
            double mn, mx;
            if (iss >> mn >> mx) {
                cm.set_limits(mn, mx);
                std::ostringstream o;
                o << "\xe2\x9c\x85 \xd0\x9b\xd0\xb8\xd0\xbc\xd0\xb8\xd1\x82\xd1\x8b: " << mn << " \xe2\x80\x94 " << mx;
                send(chat_id, 0, o.str());
                std::cout << "[bot] Limits " << mn << "-" << mx << "\n";
            } else {
                send(chat_id, 0, "\xe2\x9d\x8c \xd0\xa4\xd0\xbe\xd1\x80\xd0\xbc\xd0\xb0\xd1\x82: /limit <min> <max>");
            }
            return true;
        }
    }

    // Auto-reply (with reply to the original message)
    auto cfg = cm.get();
    if (!cfg.bot_active || cfg.answer_text.empty()) return false;
    if (cfg.limit_min == 0.0 && cfg.limit_max == 0.0) return false;

    bool monitored = false;
    for (auto id : cfg.monitored_chats)
        if (id == chat_id) { monitored = true; break; }
    if (!monitored) {
        return false;
    }

    auto amount = extract_amount(text);
    if (!amount) {
        return false;
    }

    std::cout << "[check] amount=" << *amount << " limits=[" << cfg.limit_min << "," << cfg.limit_max << "]\n";

    if (in_range(*amount, cfg.limit_min, cfg.limit_max)) {
        send(chat_id, msg_id, cfg.answer_text);  // SEND FIRST, log after
        std::cout << "[bot] MATCH " << *amount << " in chat " << chat_id << "\n";
        return true;
    }
    return false;
}
