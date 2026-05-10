#pragma once
#include <string>
#include <optional>

std::optional<double> extract_amount(const std::string& text);
bool in_range(double val, double min_v, double max_v);
