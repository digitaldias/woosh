#pragma once

#include <optional>
#include <string>
#include "audio/AudioClip.h"

class WavCodec final {
public:
    [[nodiscard]] std::optional<AudioClip> read(const std::string& path);
    [[nodiscard]] bool write(const std::string& path, const AudioClip& clip);
};



