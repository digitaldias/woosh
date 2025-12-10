#include "FileScanner.h"
#include <filesystem>

std::vector<std::string> FileScanner::scan(const std::string& folder) const {
    namespace fs = std::filesystem;
    std::vector<std::string> results;
    if (!fs::exists(folder)) return results;
    for (auto& entry : fs::recursive_directory_iterator(folder)) {
        if (!entry.is_regular_file()) continue;
        auto ext = entry.path().extension().string();
        if (ext == ".wav" || ext == ".WAV" || ext == ".mp3" || ext == ".MP3") {
            results.push_back(entry.path().string());
        }
    }
    return results;
}


