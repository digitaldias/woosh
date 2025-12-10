/**
 * @file Mp3Codec.cpp
 * @brief MP3 decoding using mpg123 library.
 */

#include "Mp3Codec.h"
#include <mpg123.h>
#include <vector>
#include <memory>

Mp3Codec::Mp3Codec() {
    initialized_ = mpg123_init() == MPG123_OK;
}

Mp3Codec::~Mp3Codec() {
    if (initialized_) mpg123_exit();
}

std::optional<AudioClip> Mp3Codec::read(const std::string& path) {
    if (!initialized_) return std::nullopt;

    int err = MPG123_OK;
    mpg123_handle* mh = mpg123_new(nullptr, &err);
    if (!mh || err != MPG123_OK) return std::nullopt;

    std::unique_ptr<mpg123_handle, decltype(&mpg123_delete)> handle(mh, &mpg123_delete);

    // Set output format to float BEFORE opening
    mpg123_param(handle.get(), MPG123_FLAGS, MPG123_FORCE_FLOAT | MPG123_GAPLESS, 0);

    // Open the file
    if (mpg123_open(handle.get(), path.c_str()) != MPG123_OK) {
        return std::nullopt;
    }

    // Scan the file to get accurate length
    mpg123_scan(handle.get());

    // Get the format
    long rate = 0;
    int channels = 0, encoding = 0;
    if (mpg123_getformat(handle.get(), &rate, &channels, &encoding) != MPG123_OK) {
        return std::nullopt;
    }

    // Ensure we get float output
    mpg123_format_none(handle.get());
    if (mpg123_format(handle.get(), rate, channels, MPG123_ENC_FLOAT_32) != MPG123_OK) {
        return std::nullopt;
    }

    // Get total length
    off_t totalSamples = mpg123_length(handle.get());
    std::vector<float> samples;
    if (totalSamples > 0) {
        samples.reserve(static_cast<size_t>(totalSamples * channels));
    }

    // Read all samples
    const size_t bufferSize = 16384;
    std::vector<unsigned char> buffer(bufferSize);
    size_t done = 0;

    while ((err = mpg123_read(handle.get(), buffer.data(), bufferSize, &done)) == MPG123_OK || err == MPG123_NEW_FORMAT) {
        if (done > 0) {
            size_t floatCount = done / sizeof(float);
            const float* fPtr = reinterpret_cast<const float*>(buffer.data());
            samples.insert(samples.end(), fPtr, fPtr + floatCount);
        }

        // If format changed, update it
        if (err == MPG123_NEW_FORMAT) {
            mpg123_getformat(handle.get(), &rate, &channels, &encoding);
        }
    }

    // Handle end of file
    if (err != MPG123_DONE && err != MPG123_OK) {
        // Read what we could, might still be usable
        if (samples.empty()) {
            return std::nullopt;
        }
    }

    // Normalize samples if they seem to be in wrong range
    // (some decoders output 16-bit range instead of -1 to 1)
    float maxAbs = 0.0f;
    for (float s : samples) {
        float abs = s < 0 ? -s : s;
        if (abs > maxAbs) maxAbs = abs;
    }

    // If samples are outside -1 to 1 range significantly, normalize them
    if (maxAbs > 1.5f) {
        float scale = 1.0f / maxAbs;
        for (float& s : samples) {
            s *= scale;
        }
    }

    return AudioClip(path, static_cast<int>(rate), channels, std::move(samples));
}
