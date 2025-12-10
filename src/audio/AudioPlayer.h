/**
 * @file AudioPlayer.h
 * @brief Audio playback using Qt Multimedia.
 *
 * Provides simple play/pause/stop functionality for AudioClip objects,
 * with position tracking for waveform playhead display.
 */

#pragma once

#include <QObject>
#include <QAudioFormat>
#include <QBuffer>
#include <memory>

class QAudioSink;
class AudioClip;

/**
 * @class AudioPlayer
 * @brief Plays AudioClip data through the default audio output.
 *
 * Uses Qt 6 QAudioSink to stream float samples converted to 16-bit PCM.
 * Emits position updates for playhead synchronization.
 */
class AudioPlayer : public QObject {
    Q_OBJECT

public:
    /**
     * @brief Playback state enumeration.
     */
    enum class State {
        Stopped,
        Playing,
        Paused
    };

    explicit AudioPlayer(QObject* parent = nullptr);
    ~AudioPlayer() override;

    /**
     * @brief Set the clip to play.
     * @param clip Pointer to the AudioClip (must remain valid during playback).
     */
    void setClip(AudioClip* clip);

    /**
     * @brief Get the current clip.
     */
    AudioClip* clip() const { return clip_; }

    /**
     * @brief Get current playback state.
     */
    State state() const { return state_; }

    /**
     * @brief Check if currently playing.
     */
    bool isPlaying() const { return state_ == State::Playing; }

    /**
     * @brief Get current playback position in frames.
     */
    int positionFrame() const { return positionFrame_; }

    /**
     * @brief Get playback duration in frames.
     */
    int durationFrames() const;

public Q_SLOTS:
    /**
     * @brief Start playback from current position.
     */
    void play();

    /**
     * @brief Pause playback (retains position).
     */
    void pause();

    /**
     * @brief Stop playback and reset position to start.
     */
    void stop();

    /**
     * @brief Toggle between play and pause.
     */
    void togglePlayPause();

    /**
     * @brief Seek to a specific frame position.
     * @param frame Frame position to seek to.
     */
    void seek(int frame);

    /**
     * @brief Set playback start/end region (for trim preview).
     * @param startFrame Start frame (0 for beginning).
     * @param endFrame End frame (0 for full clip).
     */
    void setPlaybackRegion(int startFrame, int endFrame);

Q_SIGNALS:
    /**
     * @brief Emitted periodically during playback with current position.
     * @param frame Current playback position in frames.
     */
    void positionChanged(int frame);

    /**
     * @brief Emitted when playback state changes.
     * @param state New playback state.
     */
    void stateChanged(AudioPlayer::State state);

    /**
     * @brief Emitted when playback reaches the end.
     */
    void finished();

private Q_SLOTS:
    void onAudioStateChanged(int state);
    void updatePosition();

private:
    void setupAudioOutput();
    void prepareBuffer();
    void cleanupAudioOutput();

    AudioClip* clip_ = nullptr;
    State state_ = State::Stopped;

    // Audio output
    std::unique_ptr<QAudioSink> audioSink_;
    std::unique_ptr<QBuffer> audioBuffer_;
    QByteArray pcmData_;

    // Playback position
    int positionFrame_ = 0;
    int regionStartFrame_ = 0;
    int regionEndFrame_ = 0;  // 0 = use full clip

    // Position update timer
    class QTimer* positionTimer_ = nullptr;

    // Bytes per frame for position calculation
    int bytesPerFrame_ = 0;
};

