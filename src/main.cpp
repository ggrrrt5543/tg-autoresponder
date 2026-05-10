#include "config.h"
#include "handler.h"
#include "json.hpp"

#include <iostream>
#include <string>
#include <cstdint>
#include <cstdlib>
#include <atomic>
#include <chrono>

#ifdef _WIN32
#include <windows.h>
#else
#include <dlfcn.h>
#endif

using json = nlohmann::json;

// --- TDLib JSON C API function pointers (loaded from tdjson.dll) ---
typedef void* (*td_json_client_create_t)();
typedef void  (*td_json_client_send_t)(void* client, const char* request);
typedef const char* (*td_json_client_receive_t)(void* client, double timeout);
typedef const char* (*td_json_client_execute_t)(void* client, const char* request);
typedef void  (*td_json_client_destroy_t)(void* client);

static td_json_client_create_t  fn_create  = nullptr;
static td_json_client_send_t    fn_send    = nullptr;
static td_json_client_receive_t fn_receive = nullptr;
static td_json_client_execute_t fn_execute = nullptr;
static td_json_client_destroy_t fn_destroy = nullptr;

static bool load_tdjson() {
#ifdef _WIN32
    HMODULE lib = LoadLibraryA("tdjson.dll");
    if (!lib) {
        std::cerr << "[error] Cannot load tdjson.dll! Place it next to the exe.\n";
        return false;
    }
    fn_create  = (td_json_client_create_t) GetProcAddress(lib, "td_json_client_create");
    fn_send    = (td_json_client_send_t)   GetProcAddress(lib, "td_json_client_send");
    fn_receive = (td_json_client_receive_t)GetProcAddress(lib, "td_json_client_receive");
    fn_execute = (td_json_client_execute_t)GetProcAddress(lib, "td_json_client_execute");
    fn_destroy = (td_json_client_destroy_t)GetProcAddress(lib, "td_json_client_destroy");
#else
    void* lib = dlopen("libtdjson.so", RTLD_NOW);
    if (!lib) {
        std::cerr << "[error] Cannot load libtdjson.so: " << dlerror() << "\n";
        return false;
    }
    fn_create  = (td_json_client_create_t) dlsym(lib, "td_json_client_create");
    fn_send    = (td_json_client_send_t)   dlsym(lib, "td_json_client_send");
    fn_receive = (td_json_client_receive_t)dlsym(lib, "td_json_client_receive");
    fn_execute = (td_json_client_execute_t)dlsym(lib, "td_json_client_execute");
    fn_destroy = (td_json_client_destroy_t)dlsym(lib, "td_json_client_destroy");
#endif

    if (!fn_create || !fn_send || !fn_receive || !fn_execute || !fn_destroy) {
        std::cerr << "[error] tdjson library missing required exports!\n";
        return false;
    }
    return true;
}

// --- Globals ---
static void* client = nullptr;
static int64_t my_user_id = 0;
static std::atomic<uint64_t> request_id{0};
static std::atomic<int64_t> last_ping_ms{0};

// Performance stats
static std::atomic<int64_t> best_ping_ms{9999};
static std::atomic<int64_t> total_ping_ms{0};
static std::atomic<int64_t> ping_count{0};

PerfStats get_perf_stats() {
    PerfStats s;
    s.last = last_ping_ms.load();
    s.best = best_ping_ms.load();
    s.count = ping_count.load();
    s.avg = s.count > 0 ? (double)total_ping_ms.load() / s.count : 0.0;
    return s;
}

int64_t get_last_ping_ms() { return last_ping_ms.load(); }

static void record_ping(int64_t ms) {
    last_ping_ms.store(ms);
    total_ping_ms.fetch_add(ms);
    ping_count.fetch_add(1);
    int64_t cur_best = best_ping_ms.load();
    while (ms < cur_best && !best_ping_ms.compare_exchange_weak(cur_best, ms));
}

static uint64_t next_id() { return ++request_id; }

// Send request to TDLib
static void td_send(const json& req) {
    std::string s = req.dump();
    fn_send(client, s.c_str());
}

// Send message to a chat (with optional reply)
static void send_message(int64_t chat_id, int64_t reply_to_msg_id, const std::string& text) {
    json req = {
        {"@type", "sendMessage"},
        {"chat_id", chat_id},
        {"input_message_content", {
            {"@type", "inputMessageText"},
            {"text", {
                {"@type", "formattedText"},
                {"text", text}
            }}
        }},
        {"@extra", next_id()}
    };

    // Add reply if msg_id provided
    if (reply_to_msg_id != 0) {
        req["reply_to"] = {
            {"@type", "inputMessageReplyToMessage"},
            {"message_id", reply_to_msg_id}
        };
    }

    td_send(req);
}

// --- Helper: get value from env var or stdin ---
static std::string get_input(const char* env_name, const char* prompt) {
    const char* env_val = std::getenv(env_name);
    if (env_val && env_val[0] != '\0') {
        std::cout << prompt << " [from env " << env_name << "]\n";
        return std::string(env_val);
    }
    std::string val;
    std::cout << prompt;
    std::getline(std::cin, val);
    return val;
}

// --- Auth flow ---
static void process_auth_state(const json& state) {
    std::string type = state.value("@type", "");

    if (type == "authorizationStateWaitTdlibParameters") {
        // api_id/api_hash from env or hardcoded
        int api_id = 26779626;
        std::string api_hash = "4d577c366366da914d6615417ab5ebdd";
        const char* env_id = std::getenv("TG_API_ID");
        const char* env_hash = std::getenv("TG_API_HASH");
        if (env_id) api_id = std::atoi(env_id);
        if (env_hash) api_hash = env_hash;

        json req = {
            {"@type", "setTdlibParameters"},
            {"database_directory", "tdlib_session"},
            {"use_message_database", true},
            {"use_secret_chats", false},
            {"api_id", api_id},
            {"api_hash", api_hash},
            {"system_language_code", "ru"},
            {"device_model", "Server"},
            {"application_version", "1.0"},
            {"@extra", next_id()}
        };
        td_send(req);
    }
    else if (type == "authorizationStateWaitPhoneNumber") {
        std::string phone = get_input("TG_PHONE", "Phone number: ");
        if (phone.empty()) {
            std::cerr << "[auth] No phone number! Set TG_PHONE env var.\n";
            return;
        }
        json req = {
            {"@type", "setAuthenticationPhoneNumber"},
            {"phone_number", phone},
            {"@extra", next_id()}
        };
        td_send(req);
    }
    else if (type == "authorizationStateWaitCode") {
        std::string code = get_input("TG_CODE", "Auth code: ");
        if (code.empty()) {
            std::cerr << "[auth] No code! Set TG_CODE env var and restart.\n";
            return;
        }
        json req = {
            {"@type", "checkAuthenticationCode"},
            {"code", code},
            {"@extra", next_id()}
        };
        td_send(req);
    }
    else if (type == "authorizationStateWaitPassword") {
        std::string pass = get_input("TG_2FA", "2FA password: ");
        if (pass.empty()) {
            std::cerr << "[auth] No 2FA password! Set TG_2FA env var.\n";
            return;
        }
        json req = {
            {"@type", "checkAuthenticationPassword"},
            {"password", pass},
            {"@extra", next_id()}
        };
        td_send(req);
    }
    else if (type == "authorizationStateReady") {
        std::cout << "[auth] Success!\n";
        json req = {{"@type", "getMe"}, {"@extra", 999999}};
        td_send(req);
    }
    else if (type == "authorizationStateClosed") {
        std::cout << "[auth] Closed.\n";
        exit(0);
    }
}

// --- Process one update/response ---
static void process_update(const json& upd) {
    std::string type = upd.value("@type", "");

    // Auth state updates
    if (type == "updateAuthorizationState") {
        if (upd.contains("authorization_state"))
            process_auth_state(upd["authorization_state"]);
        return;
    }

    // Response to getMe
    if (type == "user" && upd.contains("@extra") && upd["@extra"] == 999999) {
        my_user_id = upd.value("id", (int64_t)0);
        std::cout << "[auth] My ID: " << my_user_id << "\n";
        return;
    }

    // New message
    if (type == "updateNewMessage") {
        if (!upd.contains("message")) return;
        auto& msg = upd["message"];

        int64_t chat_id = msg.value("chat_id", (int64_t)0);
        int64_t msg_id = msg.value("id", (int64_t)0);

        // Get text from messageText or from forwarded channel posts
        std::string text;
        if (msg.contains("content")) {
            auto& content = msg["content"];
            std::string content_type = content.value("@type", "");
            if (content_type == "messageText") {
                if (content.contains("text") && content["text"].contains("text"))
                    text = content["text"]["text"].get<std::string>();
            }
        }
        if (text.empty()) return;

        // Log every incoming message for debugging
        std::string short_text = text.substr(0, 80);
        std::cout << "[msg] chat_id=" << chat_id << " text=\"" << short_text << "\"\n";

        // Get sender
        int64_t sender_id = 0;
        if (msg.contains("sender_id")) {
            auto& sender = msg["sender_id"];
            if (sender.value("@type", "") == "messageSenderUser")
                sender_id = sender.value("user_id", (int64_t)0);
            else if (sender.value("@type", "") == "messageSenderChat")
                sender_id = sender.value("chat_id", (int64_t)0);
        }

        // Measure ping: time from receiving update to sending reply
        auto t_start = std::chrono::steady_clock::now();

        auto send_with_ping = [&](int64_t cid, int64_t reply_id, const std::string& reply_text) {
            send_message(cid, reply_id, reply_text);
            auto t_end = std::chrono::steady_clock::now();
            auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(t_end - t_start).count();
            record_ping(ms);
            std::cout << "[ping] " << ms << "ms\n";
        };

        handle_message(chat_id, msg_id, sender_id, my_user_id, text, send_with_ping);
    }
}

// --- Main loop ---
int main() {
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
#endif
    std::cout << "=== TG Autoresponder (TDLib C++) ===\n";

    if (!load_tdjson()) return 1;

    // Set log level to minimum
    json log_req = {{"@type", "setLogVerbosityLevel"}, {"new_verbosity_level", 1}};
    fn_execute(nullptr, log_req.dump().c_str());

    client = fn_create();
    if (!client) {
        std::cerr << "[error] Failed to create TDLib client\n";
        return 1;
    }

    ConfigManager::instance().load();

    std::cout << "[main] Listening for updates...\n";

    // Ultra-fast event loop: 1ms receive timeout for minimum latency
    while (true) {
        const char* result = fn_receive(client, 0.001);
        if (!result) continue;

        try {
            json upd = json::parse(result);
            process_update(upd);
        } catch (const std::exception& e) {
            // Ignore parse errors on malformed responses
        }
    }

    fn_destroy(client);
    return 0;
}
