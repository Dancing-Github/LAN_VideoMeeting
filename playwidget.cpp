#include "playwidget.h"
#include <QOpenGLShaderProgram>
#include <QOpenGLTexture>
#include <QDebug>
#define VERTEXIN 0
#define TEXTUREIN 1


PlayWidget::PlayWidget(QWidget *parent)
    : QOpenGLWidget{parent}
{
    /*this->resize(720/2,1280/2);

    videoW = 720;
    videoH = 1280;*/
}

PlayWidget::~PlayWidget()
{
    makeCurrent();
    vbo.destroy();
    textureY->destroy();
    textureU->destroy();
    textureV->destroy();
    doneCurrent();
    if(DecodeThread::out_buffer)
    {
        av_free(DecodeThread::out_buffer);
    }
    if(last_yuvPtr)
        av_free(last_yuvPtr);
    if(yuvPtr)
        av_free(yuvPtr);

}

void PlayWidget::initWH(uint width,uint height)
{
    //this->resize(width,height);
    //videoW = width;
    //videoH = height;
}




void PlayWidget::slotShowYuv(uchar *ptr, uint width, uint height)
{

    last_yuvPtr=yuvPtr;

    if(isinit && last_yuvPtr)
    {
        av_free(last_yuvPtr);
        last_yuvPtr=nullptr;
    }

    if(!isinit)
    {
        initWH(width,height);
        isinit=true;
    }

    /*if(yuvPtr)
    {
        DecodeThread::video_mutex.lock();
        DecodeThread::video_empty++;
        if(DecodeThread::video_empty==12)
            DecodeThread::video_noEmpty.wakeOne();
        DecodeThread::video_mutex.unlock();
    }*/

    yuvPtr = ptr;
    videoW = width;
    videoH = height;

    if(!DecodeThread::isFinsh)
        update();

}


void PlayWidget::initializeGL()
{
    initializeOpenGLFunctions();
    glEnable(GL_DEPTH_TEST);

    static const GLfloat vertices[]{
        //顶点坐标
        -1.0f,-1.0f,
        -1.0f,+1.0f,
        +1.0f,+1.0f,
        +1.0f,-1.0f,
        //纹理坐标
        0.0f,1.0f,
        0.0f,0.0f,
        1.0f,0.0f,
        1.0f,1.0f,
    };


    vbo.create();
    vbo.bind();
    vbo.allocate(vertices,sizeof(vertices));
    //初始化顶点着色器对象
    QOpenGLShader *vshader = new QOpenGLShader(QOpenGLShader::Vertex,this);
    const char *vsrc =
            "attribute vec4 vertexIn; \
            attribute vec2 textureIn; \
    varying vec2 textureOut;  \
    void main(void)           \
    {                         \
        gl_Position = vertexIn; \
        textureOut = textureIn; \
    }";
    vshader->compileSourceCode(vsrc);	//编译顶点着色器程序
    //初始化片段着色器 功能gpu中yuv转换成rgb
    QOpenGLShader *fshader = new QOpenGLShader(QOpenGLShader::Fragment,this);
    //片段着色器源码
    const char *fsrc =
        #if defined(WIN32)		//windows下opengl es 需要加上float这句话
            "#ifdef GL_ES\n"
            "precision mediump float;\n"
            "#endif\n"
        #endif
            "varying vec2 textureOut; \
            uniform sampler2D tex_y; \
    uniform sampler2D tex_u; \
    uniform sampler2D tex_v; \
    void main(void) \
    { \
        vec3 yuv; \
        vec3 rgb; \
        yuv.x = texture2D(tex_y, textureOut).r; \
        yuv.y = texture2D(tex_u, textureOut).r - 0.5; \
        yuv.z = texture2D(tex_v, textureOut).r - 0.5; \
        rgb = mat3( 1,       1,         1, \
                    0,       -0.39465,  2.03211, \
                    1.13983, -0.58060,  0) * yuv; \
        gl_FragColor = vec4(rgb, 1); \
    }";
    fshader->compileSourceCode(fsrc);   //将glsl源码送入编译器编译着色器程序
    //用于绘制矩形
    program = new QOpenGLShaderProgram(this);
    program->addShader(vshader);	//将顶点着色器添加到程序容器
    program->addShader(fshader);	//将片段着色器添加到程序容器
    //绑定属性vertexIn到指定位置ATTRIB_VERTEX,该属性在顶点着色源码其中有声明
    program->bindAttributeLocation("vertexIn",VERTEXIN);
    //绑定属性textureIn到指定位置ATTRIB_TEXTURE,该属性在顶点着色源码其中有声明
    program->bindAttributeLocation("textureIn",TEXTUREIN);
    program->link();        //链接所有所有添入到的着色器程序
    program->bind();        //激活所有链接
    program->enableAttributeArray(VERTEXIN);
    program->enableAttributeArray(TEXTUREIN);
    program->setAttributeBuffer(VERTEXIN,GL_FLOAT,0,2,2*sizeof(GLfloat));
    program->setAttributeBuffer(TEXTUREIN,GL_FLOAT,8*sizeof(GLfloat),2,2*sizeof(GLfloat));

    //读取着色器中的数据变量tex_y, tex_u, tex_v的位置,这些变量的声明可以在片段着色器源码中可以看到
    textureUniformY = program->uniformLocation("tex_y");
    textureUniformU = program->uniformLocation("tex_u");
    textureUniformV = program->uniformLocation("tex_v");

    //分别创建y,u,v纹理对象
    textureY = new QOpenGLTexture(QOpenGLTexture::Target2D);
    textureU = new QOpenGLTexture(QOpenGLTexture::Target2D);
    textureV = new QOpenGLTexture(QOpenGLTexture::Target2D);
    textureY->create();
    textureU->create();
    textureV->create();
    idY = textureY->textureId();	//获取返回y分量的纹理索引值
    idU = textureU->textureId();	//获取返回u分量的纹理索引值
    idV = textureV->textureId();	//获取返回v分量的纹理索引值
    glClearColor(0.0,0.0,0.0,0.0);	//设置背景色为黑色
}

void PlayWidget::resizeGL(int w, int h)
{
    if(h<=0) h=1;
    /*int hei=w*videoH/videoW;
    int wid=h*videoW/videoH;
    if(hei<=h)
    {
        glViewport(0,(h-hei)/2,w,hei);
    }
    else
    {
        glViewport((w-wid)/2,0,wid,h);
    }*/
    glViewport(0,0,w,h);
}

void PlayWidget::paintGL()
{


    //------- 加载y数据纹理 -------
    glActiveTexture(GL_TEXTURE0);  //激活纹理单元GL_TEXTURE0,系统里面的
    glBindTexture(GL_TEXTURE_2D,idY); //绑定y分量纹理对象id到激活的纹理单元
    //使用内存中的数据创建真正的y分量纹理数据
    glTexImage2D(GL_TEXTURE_2D,0,GL_RED,videoW,videoH,0,GL_RED,GL_UNSIGNED_BYTE,yuvPtr);
    //https://blog.csdn.net/xipiaoyouzi/article/details/53584798 纹理参数解析
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    //------- 加载u数据纹理 -------
    glActiveTexture(GL_TEXTURE1); //激活纹理单元GL_TEXTURE1
    glBindTexture(GL_TEXTURE_2D,idU);
    //使用内存中的数据创建真正的u分量纹理数据
    glTexImage2D(GL_TEXTURE_2D,0,GL_RED,videoW >> 1, videoH >> 1,0,GL_RED,GL_UNSIGNED_BYTE,yuvPtr + videoW * videoH);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    //------- 加载v数据纹理 -------
    glActiveTexture(GL_TEXTURE2); //激活纹理单元GL_TEXTURE2
    glBindTexture(GL_TEXTURE_2D,idV);
    //使用内存中的数据创建真正的v分量纹理数据
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, videoW >> 1, videoH >> 1, 0, GL_RED, GL_UNSIGNED_BYTE, yuvPtr+videoW*videoH*5/4);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    //指定y纹理要使用新值
    glUniform1i(textureUniformY, 0);
    //指定u纹理要使用新值
    glUniform1i(textureUniformU, 1);
    //指定v纹理要使用新值
    glUniform1i(textureUniformV, 2);
    //使用顶点数组方式绘制图形
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);


}

/*void PlayWidget::testAudio()
{
    QAudioFormat format;
    format.setSampleFormat(QAudioFormat::Int16);
    format.setChannelConfig(QAudioFormat::ChannelConfigStereo);
    format.setChannelCount(2);
    format.setSampleRate(44100);
    m_audioOutput=new QAudioSink(m_devices->defaultAudioOutput(),format);
    m_mydevices=new MyDevice();
    m_audioOutput->start(m_mydevices);


}*/


const double VideoThread::SYNC_THRESHOLD = 0.01;
const double VideoThread::NOSYNC_THRESHOLD = 10.0;


VideoThread::VideoThread(QObject *parent)
    : QThread{parent}
{
    this->init();
}

void VideoThread::init()
{
    this->isinit=false;
    if(last_yuvPtr)
        av_free(last_yuvPtr);
    this->frame_last_pts=0.0;
    this->frame_current_pts=0.0;
    this->frame_last_delay=0.0;
    this->isstop=false;
    this->outFrame_num=0;
    this->bytes=0;
}

void VideoThread::run()
{    
    qDebug()<<"videoFPS"<<DecodeThread::videoFPS;
    while(!isstop)
    {
        //qDebug()<<"videothread run";
        if(DecodeThread::audio_time<0&&!isstop)
        {
            DecodeThread::audio_mutex.lock();
            DecodeThread::condition.wait(&DecodeThread::audio_mutex);
            DecodeThread::audio_mutex.unlock();
            qDebug()<<"videorun wake1";
        }
        else  if(!isstop)
        {
            if(DecodeThread::video_outBuffer.empty())
            {
                qDebug()<<"outBuffer_Empty!";
                if(DecodeThread::isFinsh)
                    break;
                DecodeThread::video_mutex.lock();
                DecodeThread::exist.wait(&DecodeThread::video_mutex);
                DecodeThread::video_mutex.unlock(); 
                qDebug()<<"videorun wake2";
            }
            if(DecodeThread::isFinsh||isstop)
                break;
            if(!DecodeThread::video_outBuffer.empty())
            {
                double delay;
                if(DecodeThread::video_outBuffer.size()<=5)
                    delay=1000/DecodeThread::videoFPS>100?1000/DecodeThread::videoFPS:100;
                else if(DecodeThread::video_outBuffer.size()<=7)
                    delay=1000/DecodeThread::videoFPS;
                else if(DecodeThread::video_outBuffer.size()<=13)
                    delay=900/DecodeThread::videoFPS;
                else if(DecodeThread::video_outBuffer.size()<=17)
                    delay=800/DecodeThread::videoFPS;
                else if(DecodeThread::video_outBuffer.size()<=19)
                    delay=700/DecodeThread::videoFPS;
                else
                    delay=600/DecodeThread::videoFPS;

                msleep(delay);

                if(delay>0)
                {
                    DecodeThread::video_mutex.lock();
                    emit updateYUV(DecodeThread::video_outBuffer.dequeue().buffer,DecodeThread::video_width,DecodeThread::video_height);
                    DecodeThread::video_mutex.unlock();
                }
                else
                {
                    DecodeThread::video_mutex.lock();
                    auto tmp=DecodeThread::video_outBuffer.dequeue().buffer;
                     DecodeThread::video_mutex.unlock();
                    qDebug()<<"跳帧" <<tmp;
                    if(tmp)
                    {
                        //qDebug()<<"1111111111111111111";
                        if(isinit && last_yuvPtr)
                        {
                            av_free(last_yuvPtr);
                            last_yuvPtr=nullptr;
                        }
                        last_yuvPtr=tmp;
                        if(!isinit)
                        {
                            isinit=true;
                        }
                    }

                }
                //DecodeThread::video_mutex.lock();
                if(DecodeThread::video_outBuffer.size()<DecodeThread::max_vCapacity/2)
                {
                    DecodeThread::enque.wakeAll();
                }
                //DecodeThread::video_mutex.unlock();

            }
        }

    }
    qDebug()<<"videorun done";
}

void VideoThread::stop()
{
    isstop=true;
}

double VideoThread::getDelay(double audio_clock,double delay,double diff)
{
    //一帧视频时间或10ms，10ms音视频差距无法察觉
    double sync_threshold = FFMAX(0.01, FFMIN(0.1, delay)) ;

    //if (!isnan(diff) && fabs(diff) < NOSYNC_THRESHOLD) // 不同步
    {
        if (diff <= -SYNC_THRESHOLD)//视频比音频慢，加快
            delay = FFMAX(0,  delay + diff);
        //视频比音频快，减慢
        else if (diff >= sync_threshold && delay > 0.1)
            delay = delay + diff;//音视频差距较大，且一帧的超过帧最常时间，一步到位
        else if (diff >= sync_threshold)
            delay = 2 * delay;//音视频差距较小，加倍延迟，逐渐缩小
    }

    return delay;
}

