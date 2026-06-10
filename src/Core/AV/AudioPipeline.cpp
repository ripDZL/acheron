#include "AudioPipeline.hpp"
#include "IAudioBackend.hpp"
#include "OpusEncoder.hpp"
#include "OpusDecoder.hpp"
#include "JitterBuffer.hpp"
#include "AudioMixer.hpp"

#include "Core/Logging.hpp"

#include <QTimer>

#include <algorithm>
#include <cmath>
#include <cstring>

#ifdef WITH_RNNOISE
#include <rnnoise.h>
#endif

namespace Acheron {
namespace Core {
namespace AV {

AudioPipeline::AudioPipeline(QObject *parent)
    : QObject(parent)
{
#ifdef WITH_RNNOISE
    rnnoiseState[0] = rnnoise_create(nullptr);
    rnnoiseState[1] = rnnoise_create(nullptr);
#endif
}

AudioPipeline::~AudioPipeline()
{
#ifdef WITH_RNNOISE
    if (rnnoiseState[0])
        rnnoise_destroy(static_cast<DenoiseState *>(rnnoiseState[0]));
    if (rnnoiseState[1])
        rnnoise_destroy(static_cast<DenoiseState *>(rnnoiseState[1]));
#endif
}

void AudioPipeline::start(IAudioBackend *backend, bool capturing)
{
    if (audioBackend)
        return;

    audioBackend = backend;
    connect(audioBackend, &IAudioBackend::audioCaptured, this, &AudioPipeline::onAudioCaptured, Qt::QueuedConnection);

    initializeEncoder();

    rmsThrottleTimer.start();
    userRmsThrottleTimer.start();

    if (capturing)
        audioBackend->startCapture();

    audioBackend->startPlayback();

    mixTimer = new QTimer(this);
    mixTimer->setTimerType(Qt::PreciseTimer);
    mixTimer->setInterval(AUDIO_FRAME_DURATION_MS);
    connect(mixTimer, &QTimer::timeout, this, &AudioPipeline::onMixTick);
    mixTimer->start();

    qCInfo(LogVoice) << "Audio pipeline started";
}

void AudioPipeline::stop()
{
    delete mixTimer;
    mixTimer = nullptr;

    if (audioBackend) {
        disconnect(audioBackend, &IAudioBackend::audioCaptured, this, &AudioPipeline::onAudioCaptured);
        audioBackend->stopCapture();
        audioBackend->stopPlayback();
    }

    speakers.clear();
    ssrcToUser.clear();
    pendingUserRms.clear();

    encoder.reset();

    audioBackend = nullptr;

    isSpeaking = false;
    vadHoldoffCounter = 0;

    qCDebug(LogVoice) << "Audio pipeline stopped";
}

void AudioPipeline::startCapture()
{
    if (audioBackend)
        audioBackend->startCapture();
}

void AudioPipeline::stopCapture()
{
    if (!audioBackend)
        return;

    audioBackend->stopCapture();

    if (isSpeaking) {
        sendTrailingSilence();
        isSpeaking = false;
        vadHoldoffCounter = 0;
        emit speakingChanged(false);
    }
}

void AudioPipeline::onAudioReceived(quint32 ssrc, uint16_t sequence, uint32_t /*timestamp*/, const QByteArray &opusData)
{
    auto it = speakers.find(ssrc);
    if (it == speakers.end()) {
        SpeakerState state;
        state.decoder = std::make_unique<OpusDecoder>();
        if (!state.decoder->init(AUDIO_SAMPLE_RATE, AUDIO_CHANNELS)) {
            qCWarning(LogVoice) << "Failed to init decoder for SSRC" << ssrc;
            return;
        }
        state.jitterBuffer = std::make_unique<JitterBuffer>();

        auto [inserted, _] = speakers.emplace(ssrc, std::move(state));
        it = inserted;
    }

    it->second.jitterBuffer->push(sequence, opusData);
}

void AudioPipeline::setDeafened(bool deafened)
{
    this->deafened = deafened;
}

void AudioPipeline::setSsrcUserId(quint32 ssrc, Snowflake userId)
{
    ssrcToUser[ssrc] = userId;
}

void AudioPipeline::removeUser(Snowflake userId)
{
    for (auto it = ssrcToUser.begin(); it != ssrcToUser.end();) {
        if (it.value() == userId) {
            speakers.erase(it.key());
            it = ssrcToUser.erase(it);
        } else {
            ++it;
        }
    }
    userVolumes.remove(userId);
    pendingUserRms.remove(userId);
}

void AudioPipeline::setUserVolume(Snowflake userId, float volume)
{
    if (volume == 1.0f)
        userVolumes.remove(userId);
    else
        userVolumes.insert(userId, volume);
}

void AudioPipeline::setInputDevice(const QByteArray &deviceId)
{
    if (audioBackend)
        audioBackend->setInputDevice(deviceId);
}

void AudioPipeline::setOutputDevice(const QByteArray &deviceId)
{
    if (audioBackend)
        audioBackend->setOutputDevice(deviceId);
}

void AudioPipeline::setInputGain(float gain)
{
    if (audioBackend)
        audioBackend->setInputGain(gain);
}

void AudioPipeline::setOutputVolume(float volume)
{
    if (audioBackend)
        audioBackend->setOutputVolume(volume);
}

void AudioPipeline::setVadThreshold(float threshold)
{
    vadThreshold = threshold;
}

void AudioPipeline::setPttMode(bool enabled)
{
    pttMode = enabled;
    qCInfo(LogVoice) << "AudioPipeline::setPttMode ->" << enabled;
    // If switching away from PTT while key is held, release gracefully
    if (!enabled && pttActive.load()) {
        pttActive.store(false, std::memory_order_relaxed);
    }
}

void AudioPipeline::setPttActive(bool active)
{
    pttActive.store(active, std::memory_order_relaxed);
    qCDebug(LogVoice) << "AudioPipeline::setPttActive ->" << active;
}

void AudioPipeline::setMixMono(bool enabled)
{
    mixMono.store(enabled, std::memory_order_relaxed);
    qCInfo(LogVoice) << "AudioPipeline::setMixMono ->" << enabled;
}

void AudioPipeline::setNoiseSuppress(bool enabled)
{
    noiseSuppress.store(enabled, std::memory_order_relaxed);
    qCInfo(LogVoice) << "AudioPipeline::setNoiseSuppress ->" << enabled;
}

#ifdef WITH_RNNOISE
void AudioPipeline::applyNoiseSuppression(QByteArray &pcm)
{
    if (!rnnoiseState[0])
        return;

    constexpr int kBlock = 480; // RNNoise operates on fixed 480-sample frames
    auto *s = reinterpret_cast<int16_t *>(pcm.data());
    const int totalSamples = pcm.size() / static_cast<int>(sizeof(int16_t));
    const int channels = AUDIO_CHANNELS;
    if (channels <= 0)
        return;
    const int perChannel = totalSamples / channels;
    const int blocks = perChannel / kBlock;
    if (blocks <= 0)
        return;

    float inbuf[kBlock];
    float outbuf[kBlock];
    for (int ch = 0; ch < channels && ch < 2; ++ch) {
        auto *st = static_cast<DenoiseState *>(rnnoiseState[ch]);
        if (!st)
            continue;
        for (int b = 0; b < blocks; ++b) {
            const int base = b * kBlock;
            // RNNoise expects float samples in int16 range (not normalised).
            for (int i = 0; i < kBlock; ++i)
                inbuf[i] = static_cast<float>(s[(base + i) * channels + ch]);
            rnnoise_process_frame(st, outbuf, inbuf);
            for (int i = 0; i < kBlock; ++i) {
                float f = outbuf[i];
                if (f > 32767.0f)
                    f = 32767.0f;
                else if (f < -32768.0f)
                    f = -32768.0f;
                s[(base + i) * channels + ch] = static_cast<int16_t>(f);
            }
        }
    }
}
#endif

void AudioPipeline::initializeEncoder()
{
    encoder = std::make_unique<OpusEncoder>();
    if (!encoder->init(AUDIO_SAMPLE_RATE, AUDIO_CHANNELS, opusApplication)) {
        qCCritical(LogVoice) << "Failed to initialize Opus encoder";
        encoder.reset();
        return;
    }
    encoder->setBitrate(opusBitrate);
    encoder->setComplexity(opusComplexity);
    encoder->setSignalType(opusSignalType);
    encoder->setFec(opusFec);
    encoder->setPacketLossPercent(opusPacketLossPercent);
}

void AudioPipeline::setOpusApplication(int application)
{
    if (opusApplication == application)
        return;

    opusApplication = application;

    if (encoder)
        initializeEncoder();
}

void AudioPipeline::setOpusBitrate(int bitrate)
{
    opusBitrate = bitrate;

    if (encoder)
        encoder->setBitrate(bitrate);
}

void AudioPipeline::setOpusComplexity(int complexity)
{
    opusComplexity = complexity;

    if (encoder)
        encoder->setComplexity(complexity);
}

void AudioPipeline::setOpusSignalType(int signalType)
{
    opusSignalType = signalType;

    if (encoder)
        encoder->setSignalType(signalType);
}

void AudioPipeline::setOpusFec(bool enabled)
{
    opusFec = enabled;

    if (encoder)
        encoder->setFec(enabled);
}

void AudioPipeline::setOpusPacketLossPercent(int percent)
{
    opusPacketLossPercent = percent;

    if (encoder)
        encoder->setPacketLossPercent(percent);
}

void AudioPipeline::onAudioCaptured(const QByteArray &pcmData)
{
    if (!encoder)
        return;

    // Noise suppression runs first and on every frame (continuously) so RNNoise
    // keeps its internal state warm and VAD/metering see the cleaned signal.
    const QByteArray *frame = &pcmData;
#ifdef WITH_RNNOISE
    QByteArray cleaned;
    if (noiseSuppress.load(std::memory_order_relaxed)) {
        cleaned = pcmData;
        applyNoiseSuppression(cleaned);
        frame = &cleaned;
    }
#endif

    // Always compute RMS so the input volume meter stays active in both modes
    const auto *samples = reinterpret_cast<const int16_t *>(frame->constData());
    int sampleCount = frame->size() / static_cast<int>(sizeof(int16_t));
    float rms = (sampleCount > 0) ? computeRms(samples, sampleCount) : 0.0f;

    bool voiceDetected;
    if (pttMode) {
        // PTT: open mic only while the key is held
        voiceDetected = pttActive.load(std::memory_order_relaxed);
    } else {
        // Normal VAD: threshold gate
        voiceDetected = (rms > vadThreshold);
    }

    if (rmsThrottleTimer.elapsed() >= RMS_EMIT_INTERVAL_MS) {
        // In PTT mode, only animate the meter while the key is held (actually
        // transmitting) so it doesn't look like an open mic when idle.
        float meterRms = (pttMode && !voiceDetected) ? 0.0f : rms;
        emit audioLevelChanged(meterRms);
        rmsThrottleTimer.restart();
    }

    if (voiceDetected) {
        vadHoldoffCounter = pttMode ? 0 : vadHoldoffFrames; // no artificial holdoff in PTT
        if (!isSpeaking) {
            isSpeaking = true;
            emit speakingChanged(true);
        }
    } else if (vadHoldoffCounter > 0) {
        vadHoldoffCounter--;
    } else if (isSpeaking) {
        sendTrailingSilence();
        isSpeaking = false;
        emit speakingChanged(false);
        return;
    }

    if (!isSpeaking)
        return;

    QByteArray pcmToEncode = *frame;
    if (mixMono.load(std::memory_order_relaxed) && AUDIO_CHANNELS == 2) {
        // Average L/R into both channels so the transmitted audio is mono
        // (keeps the stereo frame layout the encoder expects).
        auto *s = reinterpret_cast<int16_t *>(pcmToEncode.data());
        const int frames = pcmToEncode.size() / static_cast<int>(sizeof(int16_t)) / 2;
        for (int i = 0; i < frames; ++i) {
            const int mixed = (static_cast<int>(s[2 * i]) + static_cast<int>(s[2 * i + 1])) / 2;
            s[2 * i] = static_cast<int16_t>(mixed);
            s[2 * i + 1] = static_cast<int16_t>(mixed);
        }
    }

    QByteArray encoded = encoder->encode(pcmToEncode);
    if (!encoded.isEmpty())
        emit encodedAudioReady(encoded);
}

void AudioPipeline::onMixTick()
{
    if (deafened || !audioBackend)
        return;

    QVector<std::pair<QByteArray, float>> streams;

    for (auto it = speakers.begin(); it != speakers.end(); ++it) {
        quint32 ssrc = it->first;
        SpeakerState &state = it->second;

        QByteArray pcm;

        if (!state.pendingFrames.isEmpty()) {
            pcm = state.pendingFrames.takeFirst();
        } else {
            if (!state.jitterBuffer->isReady())
                continue;

            QByteArray opusData = state.jitterBuffer->pop();

            if (opusData.isEmpty()) {
                pcm = state.decoder->decodePlc();
            } else {
                QVector<QByteArray> frames = state.decoder->decode(opusData);
                if (frames.isEmpty())
                    continue;
                pcm = frames.first();
                for (int i = 1; i < frames.size(); i++)
                    state.pendingFrames.append(frames[i]);
            }
        }

        if (pcm.isEmpty())
            continue;

        // rms before gain
        auto userIt = ssrcToUser.constFind(ssrc);
        Snowflake userId;
        if (userIt != ssrcToUser.constEnd())
            userId = userIt.value();

        if (userId.isValid()) {
            const auto *samples = reinterpret_cast<const int16_t *>(pcm.constData());
            int count = pcm.size() / static_cast<int>(sizeof(int16_t));
            if (count > 0) {
                float rms = computeRms(samples, count);

                auto rmsIt = pendingUserRms.find(userId);
                if (rmsIt == pendingUserRms.end())
                    pendingUserRms.insert(userId, rms);
                else if (rms > rmsIt.value())
                    rmsIt.value() = rms;
            }
        }

        float gain = 1.0f;
        if (userId.isValid())
            gain = userVolumes.value(userId, 1.0f);

        streams.append({ pcm, gain });
    }

    // emit periodically
    if (userRmsThrottleTimer.elapsed() >= RMS_EMIT_INTERVAL_MS) {
        for (auto it = pendingUserRms.constBegin(); it != pendingUserRms.constEnd(); ++it)
            emit userAudioLevelChanged(it.key(), it.value());
        pendingUserRms.clear();
        userRmsThrottleTimer.restart();
    }

    if (streams.isEmpty())
        return;

    QByteArray mixed = AudioMixer::mix(streams);
    if (mixed.isEmpty())
        return;

    audioBackend->pushPlaybackFrame(reinterpret_cast<const int16_t *>(mixed.constData()));
}

bool AudioPipeline::detectVoiceActivity(const QByteArray &pcmFrame, float &outRms) const
{
    const auto *samples = reinterpret_cast<const int16_t *>(pcmFrame.constData());
    int count = pcmFrame.size() / static_cast<int>(sizeof(int16_t));
    if (count == 0) {
        outRms = 0.0f;
        return false;
    }

    outRms = computeRms(samples, count);
    return outRms > vadThreshold;
}

float AudioPipeline::computeRms(const int16_t *samples, int count)
{
    double sum = 0;
    for (int i = 0; i < count; i++)
        sum += static_cast<double>(samples[i]) * samples[i];
    return static_cast<float>(std::sqrt(sum / count));
}

void AudioPipeline::sendTrailingSilence()
{
    QByteArray silence(reinterpret_cast<const char *>(OPUS_SILENCE), sizeof(OPUS_SILENCE));
    for (int i = 0; i < TRAILING_SILENCE_FRAMES; i++)
        emit encodedAudioReady(silence);
}

} // namespace AV
} // namespace Core
} // namespace Acheron
