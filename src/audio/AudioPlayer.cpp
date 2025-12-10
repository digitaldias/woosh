/**
 * @file AudioPlayer.cpp
 * @brief Implementation of AudioPlayer using Qt Multimedia.
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

// ============================================================================
// Construction / Destruction
// ============================================================================

AudioPlayer::AudioPlayer(QObject* parent)
    : QObject(parent)
    , positionTimer_(new QTimer(this))
{
    // Position update timer (every 30ms for smooth playhead)
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
        // Resume from pause
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
        audioBuffer_->seek(bytePos);
    }

    Q_EMIT positionChanged(positionFrame_);
}

void AudioPlayer::setPlaybackRegion(int startFrame, int endFrame) {
    regionStartFrame_ = startFrame;
    regionEndFrame_ = endFrame;

    // Reset position if outside new region
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

    // Configure audio format (16-bit signed PCM)
    QAudioFormat format;
    format.setSampleRate(clip_->sampleRate());
    format.setChannelCount(clip_->channels());
    format.setSampleFormat(QAudioFormat::Int16);

    // Get default audio output device
    QAudioDevice device = QMediaDevices::defaultAudioOutput();

    if (!device.isFormatSupported(format)) {
        // Try with common fallback
        format.setSampleRate(44100);
        format.setChannelCount(2);

        if (!device.isFormatSupported(format)) {
            qWarning("Audio format not supported by device");
            return;
        }
    }

    audioSink_ = std::make_unique<QAudioSink>(device, format, this);
    audioBuffer_ = std::make_unique<QBuffer>(&pcmData_, this);

    // Calculate bytes per frame for position tracking
    bytesPerFrame_ = format.channelCount() * sizeof(qint16);

    // Connect state change signal
    connect(audioSink_.get(), &QAudioSink::stateChanged,
            this, &AudioPlayer::onAudioStateChanged);
}

void AudioPlayer::prepareBuffer() {
    if (!clip_) return;

    const auto& samples = clip_->samples();
    int channels = clip_->channels();
    size_t frameCount = clip_->frameCount();

    // Calculate region bounds
    size_t startFrame = static_cast<size_t>(regionStartFrame_);
    size_t endFrame = (regionEndFrame_ > 0)
        ? static_cast<size_t>(regionEndFrame_)
        : frameCount;

    startFrame = std::min(startFrame, frameCount);
    endFrame = std::min(endFrame, frameCount);

    if (endFrame <= startFrame) {
        pcmData_.clear();
        return;
    }

    size_t regionFrames = endFrame - startFrame;
    size_t sampleCount = regionFrames * channels;

    // Convert float [-1, 1] to 16-bit signed PCM
    pcmData_.resize(static_cast<int>(sampleCount * sizeof(qint16)));
    qint16* pcmPtr = reinterpret_cast<qint16*>(pcmData_.data());

    size_t startSample = startFrame * channels;
    for (size_t i = 0; i < sampleCount; ++i) {
        float val = samples[startSample + i];
        // Clamp and convert to 16-bit
        val = std::clamp(val, -1.0f, 1.0f);
        pcmPtr[i] = static_cast<qint16>(val * 32767.0f);
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
        // Playback finished
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

    // Calculate position from buffer position
    qint64 bytePos = audioBuffer_->pos();
    int frameOffset = static_cast<int>(bytePos / bytesPerFrame_);
    positionFrame_ = regionStartFrame_ + frameOffset;

    Q_EMIT positionChanged(positionFrame_);
}

