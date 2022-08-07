#ifndef AUDIO_PLAY_H
#define AUDIO_PLAY_H

#include <QObject>
#include "decodethread.h"
#include <QAudioFormat>
#include "mydevice.h"
#include <QAudioSink>
#include <QByteArray>
#include <QSlider>
#include <QScopedPointer>
#include <QMediaDevices>
#include <QThread>

class AudioPlay : public QThread
{
    Q_OBJECT
public:
    explicit AudioPlay(QObject *parent = nullptr);
    void run();
    void stop();
public slots:
    void init(int sampleRate);

signals:

private:
    QAudioFormat m_format;
    QMediaDevices *m_devices = nullptr;
    QAudioSink* m_audioOutput;
    MyDevice* m_mydevices;
};

#endif // AUDIO_PLAY_H
