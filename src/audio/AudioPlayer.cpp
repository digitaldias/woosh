/**
 * @file AudioPlayer.cpp
 * @brief Implementation of AudioPlayer using Qt Multimedia.
 *
 * Converts float samples to 16-bit PCM and streams to default audio output.
 */

#include "AudioPlayer.h"
#include "AudioClip.h"

#include <QAudioSink>
#include <QAudioDevice>
#include <QMediaDevices>
#include <QBuffer>
#include <QTimer>
#include <QtMath>
#include <algorithm>
#include <cmath>

// ============================================================================
// Construction / Destruction
// ============================================================================

AudioPlayer::AudioPlayer(QObject* parent)
    : QObject(parent)
    , positionTimer_(new QTimer(this))
{
    positionTimer_->setInterval(30);
    connect(positionTimer_, &QTimer::timeout, this, &AudioPlayer::updatePosition);
}

AudioPlayer::~AudioPlayer() {
    stop();
    cleanupAudioOutput();
}

// ============================================================================
// Clip management
// ============================================================================

void AudioPlayer::setClip(AudioClip* clip) {
    if (state_ != State::Stopped) {
        stop();
    }

    clip_ = clip;
    positionFrame_ = 0;
    regionStartFrame_ = 0;
    regionEndFrame_ = 0;

    cleanupAudioOutput();
}

int AudioPlayer::durationFrames() const {
    if (!clip_) return 0;
    return static_cast<int>(clip_->frameCount());
}

// ============================================================================
// Playback control
// ============================================================================

void AudioPlayer::play() {
    if (!clip_ || clip_->samples().empty()) return;

    if (state_ == State::Paused && audioSink_) {
        audioSink_->resume();
        state_ = State::Playing;
        positionTimer_->start();
        Q_EMIT stateChanged(state_);
        return;
    }

    // Start new playback
    setupAudioOutput();
    if (!audioSink_) return;

    prepareBuffer();
    audioBuffer_->open(QIODevice::ReadOnly);

    // Seek to current position in buffer
    qint64 bytePos = static_cast<qint64>(positionFrame_ - regionStartFrame_) * bytesPerFrame_;
    bytePos = std::max(qint64(0), bytePos);
    audioBuffer_->seek(bytePos);

    audioSink_->start(audioBuffer_.get());
    state_ = State::Playing;
    positionTimer_->start();

    Q_EMIT stateChanged(state_);
}

void AudioPlayer::pause() {
    if (state_ != State::Playing) return;

    if (audioSink_) {
        audioSink_->suspend();
    }

    positionTimer_->stop();
    state_ = State::Paused;
    Q_EMIT stateChanged(state_);
}

void AudioPlayer::stop() {
    positionTimer_->stop();

    if (audioSink_) {
        audioSink_->stop();
    }

    if (audioBuffer_) {
        audioBuffer_->close();
    }

    positionFrame_ = regionStartFrame_;
    state_ = State::Stopped;

    Q_EMIT positionChanged(positionFrame_);
    Q_EMIT stateChanged(state_);
}

void AudioPlayer::togglePlayPause() {
    if (state_ == State::Playing) {
        pause();
    } else {
        play();
    }
}

void AudioPlayer::seek(int frame) {
    if (!clip_) return;

    int maxFrame = static_cast<int>(clip_->frameCount());
    int effectiveStart = regionStartFrame_;
    int effectiveEnd = (regionEndFrame_ > 0) ? regionEndFrame_ : maxFrame;

    positionFrame_ = std::clamp(frame, effectiveStart, effectiveEnd);

    if (state_ == State::Playing && audioBuffer_) {
        qint64 bytePos = static_cast<qint64>(positionFrame_ - regionStartFrame_) * bytesPerFrame_;
        bytePos = std::max(qint64(0), bytePos);
        audioBuffer_->seek(bytePos);
    }

    Q_EMIT positionChanged(positionFrame_);
}

void AudioPlayer::setPlaybackRegion(int startFrame, int endFrame) {
    regionStartFrame_ = startFrame;
    regionEndFrame_ = endFrame;

    int maxFrame = clip_ ? static_cast<int>(clip_->frameCount()) : 0;
    int effectiveEnd = (regionEndFrame_ > 0) ? regionEndFrame_ : maxFrame;

    if (positionFrame_ < regionStartFrame_ || positionFrame_ > effectiveEnd) {
        positionFrame_ = regionStartFrame_;
    }
}

// ============================================================================
// Audio output setup
// ============================================================================

void AudioPlayer::setupAudioOutput() {
    if (!clip_) return;

    cleanupAudioOutput();

    // Use the clip's native format
    QAudioFormat format;
    format.setSampleRate(clip_->sampleRate());
    format.setChannelCount(clip_->channels());
    format.setSampleFormat(QAudioFormat::Int16);

    QAudioDevice device = QMediaDevices::defaultAudioOutput();

    // Check if our format is supported, if not try nearest
    if (!device.isFormatSupported(format)) {
        // Try the device's preferred format but keep Int16
        QAudioFormat preferred = device.preferredFormat();
        preferred.setSampleFormat(QAudioFormat::Int16);

        if (device.isFormatSupported(preferred)) {
            format = preferred;
            outputSampleRate_ = preferred.sampleRate();
            outputChannels_ = preferred.channelCount();
        } else {
            qWarning("Audio format not supported");
            return;
        }
    } else {
        outputSampleRate_ = clip_->sampleRate();
        outputChannels_ = clip_->channels();
    }

    audioSink_ = std::make_unique<QAudioSink>(device, format, this);
    audioBuffer_ = std::make_unique<QBuffer>(&pcmData_, this);

    bytesPerFrame_ = format.channelCount() * static_cast<int>(sizeof(qint16));

    connect(audioSink_.get(), &QAudioSink::stateChanged,
            this, &AudioPlayer::onAudioStateChanged);
}

void AudioPlayer::prepareBuffer() {
    if (!clip_) return;

    const auto& samples = clip_->samples();
    int srcChannels = clip_->channels();
    int srcRate = clip_->sampleRate();
    size_t frameCount = clip_->frameCount();

    // Calculate region bounds
    size_t startFrame = static_cast<size_t>(std::max(0, regionStartFrame_));
    size_t endFrame = (regionEndFrame_ > 0)
        ? static_cast<size_t>(regionEndFrame_)
        : frameCount;

    startFrame = std::min(startFrame, frameCount);
    endFrame = std::min(endFrame, frameCount);

    if (endFrame <= startFrame) {
        pcmData_.clear();
        return;
    }

    // Check if we need to resample
    bool needResample = (outputSampleRate_ != srcRate) || (outputChannels_ != srcChannels);

    if (!needResample) {
        // Direct conversion - no resampling needed
        size_t regionFrames = endFrame - startFrame;
        size_t sampleCount = regionFrames * srcChannels;

        pcmData_.resize(static_cast<int>(sampleCount * sizeof(qint16)));
        qint16* pcmPtr = reinterpret_cast<qint16*>(pcmData_.data());

        size_t startSample = startFrame * srcChannels;
        for (size_t i = 0; i < sampleCount; ++i) {
            float val = samples[startSample + i];
            val = std::clamp(val, -1.0f, 1.0f);
            pcmPtr[i] = static_cast<qint16>(val * 32767.0f);
        }
    } else {
        // Need resampling - use simple linear interpolation
        size_t srcFrames = endFrame - startFrame;
        double ratio = static_cast<double>(outputSampleRate_) / srcRate;
        size_t outFrames = static_cast<size_t>(srcFrames * ratio);
        size_t outSamples = outFrames * outputChannels_;

        pcmData_.resize(static_cast<int>(outSamples * sizeof(qint16)));
        qint16* pcmPtr = reinterpret_cast<qint16*>(pcmData_.data());

        for (size_t outFrame = 0; outFrame < outFrames; ++outFrame) {
            double srcPos = outFrame / ratio;
            size_t srcFrame = startFrame + static_cast<size_t>(srcPos);
            double frac = srcPos - std::floor(srcPos);

            for (int ch = 0; ch < outputChannels_; ++ch) {
                int srcCh = (ch < srcChannels) ? ch : 0;  // Map channels

                float val1 = 0.0f, val2 = 0.0f;
                size_t idx1 = srcFrame * srcChannels + srcCh;
                size_t idx2 = (srcFrame + 1) * srcChannels + srcCh;

                if (idx1 < samples.size()) val1 = samples[idx1];
                if (idx2 < samples.size()) val2 = samples[idx2];

                // Linear interpolation
                float val = static_cast<float>(val1 * (1.0 - frac) + val2 * frac);
                val = std::clamp(val, -1.0f, 1.0f);

                pcmPtr[outFrame * outputChannels_ + ch] = static_cast<qint16>(val * 32767.0f);
            }
        }

        // Update bytes per frame for position tracking
        bytesPerFrame_ = outputChannels_ * static_cast<int>(sizeof(qint16));
    }
}

void AudioPlayer::cleanupAudioOutput() {
    if (audioSink_) {
        audioSink_->stop();
        audioSink_.reset();
    }

    if (audioBuffer_) {
        audioBuffer_->close();
        audioBuffer_.reset();
    }

    pcmData_.clear();
}

// ============================================================================
// Slots
// ============================================================================

void AudioPlayer::onAudioStateChanged(int audioState) {
    QAudio::State state = static_cast<QAudio::State>(audioState);

    if (state == QAudio::IdleState && state_ == State::Playing) {
        positionTimer_->stop();
        positionFrame_ = regionStartFrame_;
        state_ = State::Stopped;

        Q_EMIT positionChanged(positionFrame_);
        Q_EMIT stateChanged(state_);
        Q_EMIT finished();
    }
}

void AudioPlayer::updatePosition() {
    if (!audioSink_ || !audioBuffer_ || state_ != State::Playing) return;

    qint64 bytePos = audioBuffer_->pos();
    int frameOffset = static_cast<int>(bytePos / bytesPerFrame_);

    // Account for resampling ratio if needed
    if (clip_ && outputSampleRate_ != clip_->sampleRate()) {
        double ratio = static_cast<double>(clip_->sampleRate()) / outputSampleRate_;
        frameOffset = static_cast<int>(frameOffset * ratio);
    }

    positionFrame_ = regionStartFrame_ + frameOffset;

    Q_EMIT positionChanged(positionFrame_);
}
