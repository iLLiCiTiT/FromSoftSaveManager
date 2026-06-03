#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio.h"

#include "AudioNotifier.h"


void tryInitSoundFromResource(ma_engine* engine, const QString& resourcePath, SoundInMemory& target) {
    QFile resource(resourcePath);
    if (!resource.open(QIODevice::ReadOnly)) return;

    target.data = resource.readAll();
    if (target.data.isEmpty()) return;

    if (ma_decoder_init_memory(target.data.constData(), static_cast<size_t>(target.data.size()), nullptr, &target.decoder) != MA_SUCCESS)
        return;

    if (ma_sound_init_from_data_source(engine, &target.decoder, 0, nullptr, &target.sound) != MA_SUCCESS) {
        ma_decoder_uninit(&target.decoder);
        return;
    }
    ma_sound_set_volume(&target.sound, 0.5f);

    target.isReady = true;
}


void playSound(SoundInMemory& soundData) {
    if (!soundData.isReady) return;

    ma_sound_stop(&soundData.sound);
    ma_sound_seek_to_pcm_frame(&soundData.sound, 0);
    ma_sound_start(&soundData.sound);
}


AudioNotifier::AudioNotifier() {
    ma_engine_config config = ma_engine_config_init();
    config.noAutoStart = MA_FALSE;
    if (ma_engine_init(&config, &m_engine) != MA_SUCCESS) return;

    m_isInitialized = true;
    tryInitSoundFromResource(&m_engine, ":/audio/soul_suck.wav", m_saveSound);
    tryInitSoundFromResource(&m_engine, ":/audio/ember_restored.wav", m_loadSound);
}

AudioNotifier::~AudioNotifier() {
    if (m_isInitialized) {
        if (m_saveSound.isReady) {
            ma_sound_uninit(&m_saveSound.sound);
            ma_decoder_uninit(&m_saveSound.decoder);
        }
        if (m_loadSound.isReady) {
            ma_sound_uninit(&m_loadSound.sound);
            ma_decoder_uninit(&m_loadSound.decoder);
        }
        ma_engine_uninit(&m_engine);
    }
}

void AudioNotifier::playSave() {
    if (!m_isInitialized) return;
    playSound(m_saveSound);
}

void AudioNotifier::playLoad() {
    if (!m_isInitialized) return;
    playSound(m_loadSound);
}
