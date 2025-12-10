#pragma once

#include <optional>
#include <string>
#include "audio/AudioClip.h"

class Mp3Codec {
public:
    Mp3Codec();
    ~Mp3Codec();

    std::optional<AudioClip> read(const std::string& path);

private:
    bool initialized_{false};
};


