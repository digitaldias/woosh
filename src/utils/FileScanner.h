#pragma once

#include <string>
#include <vector>

class FileScanner final {
public:
    [[nodiscard]] std::vector<std::string> scan(const std::string& folder) const;
};



