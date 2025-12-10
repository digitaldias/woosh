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
    mpg123_handle* mh = mpg123_new(nullptr, nullptr);
    if (!mh) return std::nullopt;

    std::unique_ptr<mpg123_handle, decltype(&mpg123_delete)> handle(mh, &mpg123_delete);
    if (mpg123_open(handle.get(), path.c_str()) != MPG123_OK) return std::nullopt;
    long rate = 0;
    int channels = 0, enc = 0;
    mpg123_getformat(handle.get(), &rate, &channels, &enc);
    mpg123_format_none(handle.get());
    mpg123_format(handle.get(), rate, channels, MPG123_ENC_FLOAT_32);

    std::vector<float> samples;
    const size_t bufferSize = 4096;
    std::vector<unsigned char> buffer(bufferSize);
    size_t done = 0;
    int err = MPG123_OK;
    while ((err = mpg123_read(handle.get(), buffer.data(), bufferSize, &done)) == MPG123_OK) {
        size_t floatCount = done / sizeof(float);
        auto* fPtr = reinterpret_cast<float*>(buffer.data());
        samples.insert(samples.end(), fPtr, fPtr + floatCount);
    }
    if (err != MPG123_OK && err != MPG123_DONE) {
        return std::nullopt;
    }
    return AudioClip(path, static_cast<int>(rate), channels, std::move(samples));
}



