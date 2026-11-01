#include "audio_controller.h"

#include "generated_speech_assets.h"

#include <AudioFileSourcePROGMEM.h>
#include <AudioGeneratorWAV.h>
#include <AudioOutputI2S.h>

static AudioGeneratorWAV* wavGen = nullptr;
static AudioFileSourcePROGMEM* fileSrc = nullptr;
static AudioOutputI2S* i2sOut = nullptr;

static const uint8_t* clipData(SpeechClip clip, SessionLanguage lang, size_t* len) {
    if (lang == SessionLanguage::MandarinGlitch) {
        switch (clip) {
            case SpeechClip::Boot:
                *len = speech_boot_zh_len;
                return speech_boot_zh;
            case SpeechClip::Panic:
                *len = speech_panic_zh_len;
                return speech_panic_zh;
            case SpeechClip::Overload:
                *len = speech_overload_zh_len;
                return speech_overload_zh;
            default:
                break;
        }
    } else {
        switch (clip) {
            case SpeechClip::Boot:
                *len = speech_boot_en_len;
                return speech_boot_en;
            case SpeechClip::Panic:
                *len = speech_panic_en_len;
                return speech_panic_en;
            case SpeechClip::Overload:
                *len = speech_overload_en_len;
                return speech_overload_en;
            default:
                break;
        }
    }
    *len = 0;
    return nullptr;
}

bool AudioController::begin() {
    i2sOut = new AudioOutputI2S();
    i2sOut->SetPinout(15, 2, 3);
    i2sOut->SetGain(1.0);
    wavGen = new AudioGeneratorWAV();
    ready_ = true;
    current_ = SpeechClip::None;
    playing_ = false;
    return true;
}

void AudioController::play(SpeechClip clip, SessionLanguage language, bool restart) {
    if (!ready_ || clip == SpeechClip::None) {
        return;
    }
    if (!restart && clip == current_ && playing_) {
        return;
    }

    if (wavGen && wavGen->isRunning()) {
        wavGen->stop();
    }
    if (fileSrc) {
        delete fileSrc;
        fileSrc = nullptr;
    }

    size_t len = 0;
    const uint8_t* data = clipData(clip, language, &len);
    if (!data || len == 0) {
        playing_ = false;
        current_ = clip;
        return;
    }

    fileSrc = new AudioFileSourcePROGMEM(data, len);
    if (!wavGen->begin(fileSrc, i2sOut)) {
        playing_ = false;
        return;
    }
    current_ = clip;
    playing_ = true;
}

void AudioController::update() {
    if (!ready_ || !wavGen) {
        return;
    }
    if (wavGen->isRunning()) {
        wavGen->loop();
    } else if (playing_) {
        playing_ = false;
    }
}

bool AudioController::isPlaying() const {
    return playing_ && wavGen && wavGen->isRunning();
}

bool AudioController::clipFinished(SpeechClip clip) const {
    if (!ready_) {
        return true;
    }
    if (current_ != clip) {
        return true;
    }
    return !isPlaying();
}
