#pragma once

#include <optional>
#include <string>
#include "audio/AudioClip.h"

class WavCodec {
public:
    std::optional<AudioClip> read(const std::string& path);
    bool write(const std::string& path, const AudioClip& clip);
};



