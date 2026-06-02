#pragma once

#include <QByteArray>
#include <QFile>

#include "miniaudio.h"


struct SoundInMemory {
    QByteArray data = {};
    ma_decoder decoder = {};
    ma_sound sound = {};
    bool isReady = false;
};

void tryInitSoundFromResource(ma_engine* engine, const QString& resourcePath, SoundInMemory& target);
void playSound(SoundInMemory& soundData);


class AudioNotifier {
public:
    AudioNotifier();
    ~AudioNotifier();

    void playSave();
    void playLoad();

private:
    ma_engine m_engine = {};
    bool m_isInitialized = false;
    SoundInMemory m_saveSound = {};
    SoundInMemory m_loadSound = {};
};
