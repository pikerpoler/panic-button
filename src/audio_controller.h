#pragma once

#include "panic_types.h"

#include <stdint.h>

class AudioController {
public:
    bool begin();
    void update();
    void play(SpeechClip clip, SessionLanguage language, bool restart);
    bool isPlaying() const;
    bool clipFinished(SpeechClip clip) const;

private:
    bool ready_;
    SpeechClip current_;
    bool playing_;
};
