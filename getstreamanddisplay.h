#ifndef GETSTREAMANDDISPLAY_H
#define GETSTREAMANDDISPLAY_H

#include <QWidget>
#include <QOpenGLWidget>
#include<QPainter>
#include<QTimer>
#include <QElapsedTimer>
extern "C" {

   #include "libavcodec/avcodec.h"
   #include "libavformat/avformat.h"
   #include "libswscale/swscale.h"
   #include "libavdevice/avdevice.h"
   #include "libavformat/avio.h"
   #include "libavutil/imgutils.h"
   #include "libavutil/mathematics.h"
   #include "libavutil/time.h"
   #include "libavutil/opt.h"

}

namespace Ui {
class getStreamAndDisplay;
}

class getStreamAndDisplay : public QOpenGLWidget
{
    Q_OBJECT

public:
    explicit getStreamAndDisplay(QOpenGLWidget *parent = nullptr);
    ~getStreamAndDisplay();
    void init();
    void wirteHead();
    void read();
    void endRead();
    void free();
    void display(AVPacket*);
    static DWORD WINAPI getStreamProc(LPVOID lpParam);

private:
    Ui::getStreamAndDisplay *ui;
    const AVOutputFormat* ofmt;
    AVFormatContext* ifmt_ctx;
    AVFormatContext* ofmt_ctx;
    AVPacket* pkt;
    const char* infilename;
    const char* outfilename;
    int videoindex=-1;
    int frameindex=0;
    long long firstPts=0;
    long long firstDts=0;


    AVCodecParameters* codecPar;
    AVCodecContext* codecCtx;
    AVFrame* frame;
    AVFrame* RGBframe;
    SwsContext* imgConvertContext;
    unsigned char* outbuffer;




};

#endif // GETSTREAMANDDISPLAY_H
