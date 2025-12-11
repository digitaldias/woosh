/**
 * @file Mp3Encoder.h
 * @brief MP3 encoding using LAME library with ID3 tag support.
 */

#pragma once

#include <string>
#include "audio/AudioClip.h"

/**
 * @brief ID3 tag metadata for MP3 files.
 */
struct Mp3Metadata {
    std::string title;       ///< Track title (defaults to filename)
    std::string artist;      ///< Artist/author
    std::string album;       ///< Album/game name
    std::string comment;     ///< Comment (e.g., "Made by Woosh")
    std::string year;        ///< Year of creation
};

/**
 * @brief MP3 encoder using LAME library.
 * 
 * Supports encoding AudioClip data to MP3 format with configurable
 * bitrate and ID3v2 metadata tags.
 */
class Mp3Encoder final {
public:
    /**
     * @brief Supported bitrate modes for MP3 encoding.
     */
    enum class BitrateMode {
        CBR_128 = 128,  ///< Constant 128 kbps - smallest file size
        CBR_160 = 160,  ///< Constant 160 kbps - balanced
        CBR_192 = 192,  ///< Constant 192 kbps - better quality
        VBR_HIGH = 0    ///< Variable bitrate, quality setting ~190 kbps average
    };

    Mp3Encoder() = default;
    ~Mp3Encoder() = default;

    /**
     * @brief Encode an AudioClip to MP3 format.
     * 
     * @param clip The audio clip to encode.
     * @param outputPath Full path for the output MP3 file.
     * @param bitrate The bitrate mode to use.
     * @param metadata Optional ID3 tag metadata.
     * @return true if encoding succeeded, false otherwise.
     */
    [[nodiscard]] bool encode(
        const AudioClip& clip,
        const std::string& outputPath,
        BitrateMode bitrate = BitrateMode::CBR_160,
        const Mp3Metadata& metadata = {}
    );

    /**
     * @brief Get the last error message if encoding failed.
     */
    [[nodiscard]] const std::string& lastError() const noexcept { return lastError_; }

private:
    std::string lastError_;
};
