#ifndef DECODETHREAD_H
#define DECODETHREAD_H

//当前C++兼容C语言
extern "C"
{
//avcodec:编解码(最重要的库)
#include <libavcodec/avcodec.h>
//avformat:封装格式处理
#include <libavformat/avformat.h>
//swscale:视频像素数据格式转换
#include <libswscale/swscale.h>
//avdevice:各种设备的输入输出
#include <libavdevice/avdevice.h>
//avutil:工具库（大部分库都需要这个库的支持）
#include <libavutil/avutil.h>
#include <libavutil/imgutils.h>
//音频采样数据格式库
#include <libswresample/swresample.h>
#include <libavutil/frame.h>
#include <libavutil/time.h>
}


#include <QMainWindow>
#include <QFile>
#include <QImage>
#include <QLabel>
#include <QPixmap>
#include <QtOpenGLWidgets/QOpenGLWidget>
#include <QPainter>
#include <QTime>
#include <QMutex>
#include <QObject>
#include <QThread>
#include <QQueue>
#include <QMutex>
#include <QWaitCondition>
#include <QByteArray>
#include <QVector>
#include <QString>


struct AVState
{
    AVFormatContext* av_format_context;      //封装格式上下文
    AVStream* video_stream;               //视频流
    AVStream* audio_stream;               //音频流
    int audio_stream_id=-1;               //视频流id
    int video_stream_id=-1;               //音频流id
    AVCodecContext* video_codec_context;       //视频解码器上下文
    AVCodecContext* audio_codec_context;       //音频解码器上下文
    //AVCodec* video_codec;                      //视频解码器
    //AVCodec* audio_codec;                      //音频解码器
    int y_size;
    int u_size;
    int v_size;
    int out_nb_channels;//输出的声道个数

    AVPacket* av_packet;
    AVFrame* video_inframe;
    AVFrame* video_outframe;
    AVFrame* audio_inframe;
    uint8_t* video_outbuffer;
    uint8_t* audio_outbuffer;
    SwsContext* sws_context; //转码视频
    SwrContext* swr_context;
};

struct ABuffer
{
    uchar* buffer=nullptr;
    int len=0;
    double pts=0.0;
    int bytes=0;
};

class DecodeThread : public QThread
{
    Q_OBJECT

public:
    explicit DecodeThread(QObject *parent = nullptr);
    ~DecodeThread();
    void run();
    void stop();
    void init();
    void del();
    void setFilePath(QString);
    void setType();


    static bool isFinsh;
    static uchar* out_buffer;
    static uchar* ptr;
    static QVector<ABuffer> video_outdata;
    static QVector<ABuffer> audio_outdata;
    static QQueue<ABuffer> video_outBuffer;
    static QQueue<ABuffer> audio_outBuffer;
    static QMutex video_mutex;
    static QMutex audio_mutex;
    static QWaitCondition condition;
    static QWaitCondition exist;
    static QWaitCondition enque;
    static QWaitCondition video_nofull;
    static QWaitCondition video_noEmpty;
    static QWaitCondition audio_nofull;
    static QWaitCondition audio_noEmpty;
    static uchar* audio_buffer;
    static int audio_len;
    static int MAX_AUDIO_SIZE;
    static int video_num;
    static int audio_num;
    static double delay;
    static double start_time;
    static double audio_time;
    static uint video_width;
    static uint video_height;
    static int video_full;
    static int video_empty;
    static int audio_full;
    static int audio_empty;
    static int max_videoFrame;
    static int max_audioFrame;
    static int max_vCapacity;
    static double videoFPS;

signals:
    void sigGetOneFrame(QImage);
    void sigGetOneFrame(uchar* outframe,uint width,uint height);
    void sigGetOneFrame(uint width,uint height);
    void sigGetOneFrame(double);
    void sigGetSampleRate(int);
    void sigGetSampleRate(int,bool);
    void sigGetWH(uint,uint);
    void happenFail();
private:
    int getData();
    int getInfo();
    void findInfo();
    void initVideo();
    void initAudio();
    void decodeVideo();
    void decodeAudio();
    double synchronize(AVFrame *inFrame,double pts);
    static int decode_interrupt_cb(void *ctx);
    bool m_isstop;
    bool m_isdel;
    bool m_isAinit;
    bool m_isVinit;
    bool m_type;
    char* m_filePath;
    QString m_path;
    const char* m_inFile;
    AVState* m_avstate;
    FILE* m_video_outfile;
    FILE* m_audio_outfile;
    bool flag;
    bool flag1;
    double video_clock=0.0;
    double audio_clock=0.0;
    double last_video_pts=0.0;
    int64_t m_nStartOpenTS;


};

#endif // DECODETHREAD_H
