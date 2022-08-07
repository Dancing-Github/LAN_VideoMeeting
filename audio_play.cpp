#include "audio_play.h"

AudioPlay::AudioPlay(QObject *parent)
    : QThread{parent}
{
    m_format.setSampleFormat(QAudioFormat::Int16);
    m_format.setChannelConfig(QAudioFormat::ChannelConfigStereo);
    m_format.setChannelCount(2);
    m_format.setSampleRate(44100);
    m_mydevices=new MyDevice();
}

void AudioPlay::init(int sampleRate)
{
    m_format.setSampleRate(sampleRate);
}

void AudioPlay::run()
{
    m_audioOutput=new QAudioSink(m_devices->defaultAudioOutput(),m_format);
    m_audioOutput->start(m_mydevices);
}

void AudioPlay::stop()
{

}
