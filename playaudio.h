#ifndef PLAYAUDIO_H
#define PLAYAUDIO_H

#include <QObject>
#include <QIODevice>
#include <math.h>
#include <QComboBox>
//#include <QIODevice>
#include <QLabel>
#include <QMainWindow>
//#include <QObject>
#include <QPushButton>
#include <QSlider>
#include <QTimer>
#include <QScopedPointer>
#include <QMediaDevices>
#include "decodethread.h"
#include <QAudioFormat>
#include <QAudioSink>
#include <QByteArray>
//#include <QSlider>
//#include <QScopedPointer>
//#include <QMediaDevices>
#include "playwidget.h"



class MyDevice : public QIODevice
{
    Q_OBJECT
public:
    explicit MyDevice(QObject *parent = nullptr);
    MyDevice(QAudioFormat format);
    MyDevice(QAudioFormat format,PlayWidget* playwidget);
    MyDevice(ABuffer pcm);
    ~MyDevice();
    void start();
    void stop();
    qint64 readData(char *data, qint64 maxlen)override; //重新实现的虚函数
    qint64 writeData(const char *data, qint64 len)override; //它是个纯虚函数， 不得不实现
    qint64 bytesAvailable() const override;
    qint64 size() const override { return m_buffer.size(); }
public slots:
    //void slotGetData(char*,qint64);
signals:
    void sigGetOneFrame(uchar*,double);
    void sigGetOneFrame(double);
    void sigGetOneFrame();
    void isFinish();
private:
    double getDelay(double audio_clock,double delay,double diff);
    static const double SYNC_THRESHOLD;
    static const double NOSYNC_THRESHOLD;
    qint64 m_pos = 0;
    QAudioFormat m_format;
    QByteArray m_buffer;
    ABuffer m_buffer1;
    int len_written=0; //记录已写入多少字节
    int outFrame_num=0;
    double frame_last_pts=0.0;
    double frame_last_delay=0.0;
    PlayWidget* m_playwidget;
    bool init_time=false;
};



class AudioPlay : public QObject
{
    Q_OBJECT
public:
    explicit AudioPlay(QObject *parent = nullptr);
    AudioPlay(PlayWidget*);
    ~AudioPlay();

    //void playaudio();
    MyDevice* m_mydevices;
public slots:
    void init(int sampleRate);
    void init(int sampleRate,bool type);
    void startplay();
    void playaudio();
    void stop();
signals:
    void inited();

private:
    QAudioFormat m_format;
    QMediaDevices *m_devices = nullptr;
    QAudioSink* m_audioOutput;
    PlayWidget* m_playwidget;
    bool m_isstop;
    bool m_isstart;

};

#endif // PLAYAUDIO_H
