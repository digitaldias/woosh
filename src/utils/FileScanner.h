#pragma once

#include <string>
#include <vector>

class FileScanner {
public:
    std::vector<std::string> scan(const std::string& folder) const;
};



