#include "utils.h"
#include <cstring>
#include <cstdlib>

// Fast manual search — no regex overhead
// Looks for "Итого:" or "Количество:" followed by a number
static std::optional<double> find_number_after(const std::string& text, const char* keyword) {
    const char* pos = strstr(text.c_str(), keyword);
    if (!pos) return std::nullopt;

    pos += strlen(keyword);

    // Skip whitespace
    while (*pos == ' ' || *pos == '\t' || *pos == '\n') pos++;

    // Parse number
    if ((*pos >= '0' && *pos <= '9') || *pos == '.') {
        char* end = nullptr;
        double val = strtod(pos, &end);
        if (end != pos) return val;
    }
    return std::nullopt;
}

std::optional<double> extract_amount(const std::string& text) {
    // "Итого:" in UTF-8: \xd0\x98\xd1\x82\xd0\xbe\xd0\xb3\xd0\xbe:
    auto r = find_number_after(text, "\xd0\x98\xd1\x82\xd0\xbe\xd0\xb3\xd0\xbe:");
    if (r) return r;

    // "Количество:" in UTF-8
    r = find_number_after(text, "\xd0\x9a\xd0\xbe\xd0\xbb\xd0\xb8\xd1\x87\xd0\xb5\xd1\x81\xd1\x82\xd0\xb2\xd0\xbe:");
    return r;
}

bool in_range(double val, double min_v, double max_v) {
    return val >= min_v && val <= max_v;
}
