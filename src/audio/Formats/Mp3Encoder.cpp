/**
 * @file Mp3Encoder.cpp
 * @brief MP3 encoding using LAME library with ID3 tag support.
 */

#include "Mp3Encoder.h"
#include <lame/lame.h>
#include <fstream>
#include <vector>
#include <cstring>
#include <filesystem>

bool Mp3Encoder::encode(
    const AudioClip& clip,
    const std::string& outputPath,
    BitrateMode bitrate,
    const Mp3Metadata& metadata
) {
    lastError_.clear();

    if (clip.samples().empty()) {
        lastError_ = "Cannot encode empty audio clip";
        return false;
    }

    // Initialize LAME
    lame_global_flags* gfp = lame_init();
    if (!gfp) {
        lastError_ = "Failed to initialize LAME encoder";
        return false;
    }

    // Configure encoder
    lame_set_in_samplerate(gfp, clip.sampleRate());
    lame_set_num_channels(gfp, clip.channels());
    
    // Output settings - always stereo output for compatibility
    if (clip.channels() == 1) {
        lame_set_mode(gfp, MONO);
    } else {
        lame_set_mode(gfp, JOINT_STEREO);
    }

    // Set bitrate
    if (bitrate == BitrateMode::VBR_HIGH) {
        // VBR mode with quality setting (0=best, 9=worst)
        lame_set_VBR(gfp, vbr_default);
        lame_set_VBR_quality(gfp, 2.0f);  // ~190 kbps average
    } else {
        // CBR mode - explicitly disable VBR
        lame_set_VBR(gfp, vbr_off);
        lame_set_brate(gfp, static_cast<int>(bitrate));
        // Force CBR by disabling VBR and ABR
        lame_set_VBR_mean_bitrate_kbps(gfp, static_cast<int>(bitrate));
    }

    // Quality setting (0=best/slowest, 9=worst/fastest)
    // Use 2 for high quality
    lame_set_quality(gfp, 2);

    // Configure ID3 tags - initialize first to clear any defaults
    id3tag_init(gfp);
    id3tag_add_v2(gfp);
    id3tag_v2_only(gfp);  // Only write ID3v2, not v1
    lame_set_write_id3tag_automatic(gfp, 1);
    
    // Get title from metadata or filename
    std::string title = metadata.title;
    if (title.empty()) {
        std::filesystem::path p(clip.filePath());
        title = p.stem().string();
    }
    id3tag_set_title(gfp, title.c_str());
    
    if (!metadata.artist.empty()) {
        id3tag_set_artist(gfp, metadata.artist.c_str());
    }
    if (!metadata.album.empty()) {
        id3tag_set_album(gfp, metadata.album.c_str());
    }
    if (!metadata.comment.empty()) {
        id3tag_set_comment(gfp, metadata.comment.c_str());
    }
    if (!metadata.year.empty()) {
        id3tag_set_year(gfp, metadata.year.c_str());
    }

    // Initialize encoder with settings
    if (lame_init_params(gfp) < 0) {
        lastError_ = "Failed to initialize LAME parameters";
        lame_close(gfp);
        return false;
    }

    // Open output file
    std::ofstream outFile(outputPath, std::ios::binary);
    if (!outFile.is_open()) {
        lastError_ = "Failed to open output file: " + outputPath;
        lame_close(gfp);
        return false;
    }

    // Prepare for encoding
    const std::vector<float>& samples = clip.samples();
    const int channels = clip.channels();
    const size_t totalFrames = samples.size() / channels;
    
    // Buffer for encoded MP3 data (worst case: 1.25 * samples + 7200)
    const size_t mp3BufferSize = static_cast<size_t>(1.25 * 8192 + 7200);
    std::vector<unsigned char> mp3Buffer(mp3BufferSize);

    // Process in chunks
    const size_t chunkFrames = 8192;
    size_t framesProcessed = 0;

    if (channels == 1) {
        // Mono encoding
        while (framesProcessed < totalFrames) {
            size_t framesToProcess = std::min(chunkFrames, totalFrames - framesProcessed);
            
            int bytesEncoded = lame_encode_buffer_ieee_float(
                gfp,
                samples.data() + framesProcessed,
                nullptr,  // No right channel for mono
                static_cast<int>(framesToProcess),
                mp3Buffer.data(),
                static_cast<int>(mp3BufferSize)
            );

            if (bytesEncoded < 0) {
                lastError_ = "LAME encoding error: " + std::to_string(bytesEncoded);
                lame_close(gfp);
                return false;
            }

            if (bytesEncoded > 0) {
                outFile.write(reinterpret_cast<char*>(mp3Buffer.data()), bytesEncoded);
            }

            framesProcessed += framesToProcess;
        }
    } else {
        // Stereo encoding - need to deinterleave
        std::vector<float> leftChannel(chunkFrames);
        std::vector<float> rightChannel(chunkFrames);

        while (framesProcessed < totalFrames) {
            size_t framesToProcess = std::min(chunkFrames, totalFrames - framesProcessed);
            
            // Deinterleave stereo samples
            for (size_t i = 0; i < framesToProcess; ++i) {
                size_t srcIdx = (framesProcessed + i) * 2;
                leftChannel[i] = samples[srcIdx];
                rightChannel[i] = samples[srcIdx + 1];
            }

            int bytesEncoded = lame_encode_buffer_ieee_float(
                gfp,
                leftChannel.data(),
                rightChannel.data(),
                static_cast<int>(framesToProcess),
                mp3Buffer.data(),
                static_cast<int>(mp3BufferSize)
            );

            if (bytesEncoded < 0) {
                lastError_ = "LAME encoding error: " + std::to_string(bytesEncoded);
                lame_close(gfp);
                return false;
            }

            if (bytesEncoded > 0) {
                outFile.write(reinterpret_cast<char*>(mp3Buffer.data()), bytesEncoded);
            }

            framesProcessed += framesToProcess;
        }
    }

    // Flush remaining data
    int finalBytes = lame_encode_flush(gfp, mp3Buffer.data(), static_cast<int>(mp3BufferSize));
    if (finalBytes > 0) {
        outFile.write(reinterpret_cast<char*>(mp3Buffer.data()), finalBytes);
    }

    // Write LAME/INFO tag (for accurate seeking)
    size_t lameTagSize = lame_get_lametag_frame(gfp, mp3Buffer.data(), mp3BufferSize);
    if (lameTagSize > 0) {
        // Seek to beginning and write the tag
        outFile.seekp(0);
        outFile.write(reinterpret_cast<char*>(mp3Buffer.data()), lameTagSize);
    }

    outFile.close();
    lame_close(gfp);

    return true;
}
