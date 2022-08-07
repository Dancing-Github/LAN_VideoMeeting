#ifndef SENDER_VIDEOANDAUDIO_H
#define SENDER_VIDEOANDAUDIO_H

extern "C" {
#include "libavcodec/avcodec.h"
#include "libavcodec/bsf.h"
#include "libavformat/avformat.h"
#include "libavutil/buffer.h"
#include "libswscale/swscale.h"
#include "libavdevice/avdevice.h"
#include "libavformat/avio.h"
#include "libavutil/audio_fifo.h"
#include "libavutil/imgutils.h"
#include "libswresample/swresample.h"
#include "libavutil/log.h"
#include "libavutil/avassert.h"
}

#include <QWaitCondition>
#include <QMutex>
#include <QFile>
#include <QQueue>
#include <QOpenGLWidget>
#include <QTime>
#include <QString>
#include <QFuture>
#include "playwidget.h"

class sender_VideoAndAudio : public QOpenGLWidget
{
    Q_OBJECT
public:     
    explicit sender_VideoAndAudio(QString audioDeviceName, QString rtpOutputAddress, QWidget *parent = nullptr);
    ~sender_VideoAndAudio();
    bool sender_Initialize();

private:
    int audioIndex,videoIndex,codecIndex ,videoFrameSize,localBufferSize;
    bool bCap ;
    bool outFileOpened;
    int fps=0;

    //FILE* m_video_outfile;
    uchar* out_buffer;
    QFuture<bool> ScreenProc, AudioProc, WriteProc, EncodeProc;

    QTime start;
    QWaitCondition  bufferEmpty;
    QMutex VideoSection, AudioSection, CapSection, WriteSection;
    QImage image;
    QString getResolution( int screenIndex);
    //QString getAudioDev();
    QString audioDevice;
    QString outputAddress;//格式类似于“rtp://234.234.234.234:23334"

    QQueue<AVPacket*> writeBuffer;

    bool openVideoCapture();
    bool openAudioCapture();
    bool openOutPut();
    bool ScreenCapThreadProc();
    bool AudioCapThreadProc();
    bool EncodeThreadProc();
    bool WritePacketThreadProc();

    void sendPaintEvent();

    AVFormatContext *pFormatCtx_inVideo= nullptr, *pFormatCtx_inAudio = nullptr, *pFormatCtx_Out = nullptr;
    AVCodecParameters *pCodecPra_inVideo= nullptr,*pCodecPra_inAudio= nullptr,*pCodecPra_Out= nullptr;
    AVCodecContext *pCodecCtx_inVideo= nullptr, *pCodecCtx_inAudio= nullptr,*pCodecCtx_outVideo= nullptr,*pCodecCtx_outAudio= nullptr;

    AVFrame *localVideoFrame= nullptr,*YUVframe= nullptr;
    unsigned char *localPictureBuffer= nullptr ;

    AVFifoBuffer 	*fifo_video = nullptr;
    AVFifoBuffer 	*fifo_local = nullptr;
    AVAudioFifo	*fifo_audio = nullptr;

    SwsContext *imgConvertContext= nullptr,*videoConvertContext= nullptr;
    SwrContext *audioConvertContext= nullptr;

signals:
    void sender_sendYUV(uchar *ptr,uint width,uint height);
    //void error(QString);


};

#endif // SENDER_VIDEOANDAUDIO_H
