#include "utils.h"
#include <regex>

std::optional<double> extract_amount(const std::string& text) {
    // Priority: "Итого:" then "Количество:"
    static const std::regex re_itogo(R"(\xd0\x98\xd1\x82\xd0\xbe\xd0\xb3\xd0\xbe:\s*([0-9]+(?:\.[0-9]+)?))");
    static const std::regex re_kol(R"(\xd0\x9a\xd0\xbe\xd0\xbb\xd0\xb8\xd1\x87\xd0\xb5\xd1\x81\xd1\x82\xd0\xb2\xd0\xbe:\s*([0-9]+(?:\.[0-9]+)?))");

    std::smatch m;
    if (std::regex_search(text, m, re_itogo)) {
        try { return std::stod(m[1].str()); } catch (...) {}
    }
    if (std::regex_search(text, m, re_kol)) {
        try { return std::stod(m[1].str()); } catch (...) {}
    }
    return std::nullopt;
}

bool in_range(double val, double min_v, double max_v) {
    return val >= min_v && val <= max_v;
}
