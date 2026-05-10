#pragma once
#include <string>
#include <optional>

// Extract amount from message: looks for "Итого: X" or "Количество: X"
std::optional<double> extract_amount(const std::string& text);

// Check if value is in [min, max]
bool in_range(double val, double min_v, double max_v);
