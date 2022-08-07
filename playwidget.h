#ifndef PLAYWIDGET_H
#define PLAYWIDGET_H

#include <QOpenGLWidget>
#include <QOpenGLFunctions>
#include <QOpenGLBuffer>
#include <QTimer>
#include <QPainter>
#include "decodethread.h"
#include <QAudioFormat>
#include <QAudioSink>
#include <QByteArray>
#include <QSlider>
#include <QScopedPointer>
#include <QMediaDevices>
#include <QThread>
#include <QEventLoop>




QT_FORWARD_DECLARE_CLASS(QOpenGLShaderProgram)
QT_FORWARD_DECLARE_CLASS(QOpenGLTexture)



class PlayWidget : public QOpenGLWidget,protected QOpenGLFunctions,public QPainter
{
    Q_OBJECT

public:
    explicit PlayWidget(QWidget *parent = nullptr);
    ~PlayWidget();
    //void testAudio();
public slots:
    void slotShowYuv(uchar *ptr,uint width,uint height); //显示一帧Yuv图像
//    void slotShowYuv(uchar*,double);
//    void slotShowYuv(uchar*);
//    void slotShowYuv(double);
//    void slotShowYuv();
//    void slotShowYuv(int index,uint width,uint height,int bytes);
    void initWH(uint,uint);
protected:
    void initializeGL() Q_DECL_OVERRIDE;
    void paintGL() Q_DECL_OVERRIDE;
    void resizeGL(int w, int h) override;

private:
    QOpenGLShaderProgram *program;
    QOpenGLBuffer vbo;
    GLuint textureUniformY,textureUniformU,textureUniformV; //opengl中y、u、v分量位置
    QOpenGLTexture *textureY = nullptr,*textureU = nullptr,*textureV = nullptr;
    GLuint idY,idU,idV; //自己创建的纹理对象ID，创建错误返回0
    uint videoW,videoH;
    uchar* yuvPtr=nullptr;
    uchar* last_yuvPtr=nullptr;
    bool isinit=false;
    int m_bytes=0;
    /*QMediaDevices *m_devices = nullptr;
    QAudioSink* m_audioOutput;
    MyDevice* m_mydevices;*/

};

class VideoThread:public QThread
{
    Q_OBJECT

public:
    explicit VideoThread(QObject *parent = nullptr);
    void run()override;
    void stop();
    void init();
    double getDelay(double,double,double);
signals:
    //void updateYUV();
    //void updateYUV(uchar*);
    void updateYUV(uchar*,uint,uint);
    //void updateYUV(int index,uint width,uint height,int bytes);
private:
    double frame_last_pts=0.0;
    double frame_current_pts=0.0;
    double frame_last_delay=0.0;
    bool isstop=false;
    static const double SYNC_THRESHOLD;
    static const double NOSYNC_THRESHOLD;
    int outFrame_num=0;
    uchar* yuvptr;
    uchar* last_yuvPtr=nullptr;
    bool isinit=false;
    int bytes=0;
};

#endif // PLAYWIDGET_H
