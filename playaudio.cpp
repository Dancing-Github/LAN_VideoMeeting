#include "playaudio.h"

const double MyDevice::SYNC_THRESHOLD = 0.01;
const double MyDevice::NOSYNC_THRESHOLD = 10.0;

MyDevice::MyDevice(QObject *parent)
    : QIODevice{parent}
{
    open(QIODevice::ReadOnly); // 为了解决QIODevice::read (QIODevice): device not open.
}

MyDevice::MyDevice(QAudioFormat format):m_format(format)
{
    open(QIODevice::ReadOnly);
}

MyDevice::MyDevice(QAudioFormat format,PlayWidget* playwidget):m_format(format),m_playwidget(playwidget)
{
    open(QIODevice::ReadOnly);
    connect(this,SIGNAL(sigGetOneFrame(uchar*,double)),
            m_playwidget,SLOT(slotShowYuv(uchar*,double)),Qt::QueuedConnection);
}

MyDevice::MyDevice(ABuffer pcm):m_buffer1(pcm)
{
    open(QIODevice::ReadOnly);
}

MyDevice::~MyDevice()
{
    this->stop();
}

void MyDevice::start()
{
    open(QIODevice::ReadOnly); // 为了解决QIODevice::read (QIODevice): device not open.
}

void MyDevice::stop()
{
    m_pos = 0;
    close();
    /*if(!DecodeThread::audio_buffer)
    {
        av_free(DecodeThread::audio_buffer);
    }
    while(!DecodeThread::audio_outBuffer.empty())
    {
        av_free(DecodeThread::audio_outBuffer.dequeue().buffer);
    }*/
}

qint64 MyDevice::readData(char *data, qint64 len)
{
    //qDebug()<<"readData";


    /*if(!m_buffer1.buffer && !DecodeThread::audio_outBuffer.empty())
    {
       DecodeThread::audio_mutex.lock();

       m_buffer1=DecodeThread::audio_outBuffer.dequeue();
       DecodeThread::audio_time=m_buffer1.pts;
       DecodeThread::audio_mutex.unlock();
       if(!init_time)
       {
           //DecodeThread::start_time=(double)clock();
           DecodeThread::condition.wakeOne();
           init_time=true;
       }
    }*/

    if(!m_buffer1.buffer)
    {
        DecodeThread::audio_mutex.lock();
        DecodeThread::audio_full--;
        if(DecodeThread::audio_full<0)
        {
            if(DecodeThread::isFinsh)
            {
                DecodeThread::audio_mutex.unlock();
                emit isFinish();
                return -1;
            }
            DecodeThread::audio_nofull.wait(&DecodeThread::audio_mutex);
        }
        m_buffer1.buffer=DecodeThread::audio_outdata[outFrame_num%DecodeThread::max_audioFrame].buffer;
        m_buffer1.len=DecodeThread::audio_outdata[outFrame_num%DecodeThread::max_audioFrame].len;
        m_buffer1.pts=DecodeThread::audio_outdata[outFrame_num%DecodeThread::max_audioFrame].pts;
        DecodeThread::audio_time=m_buffer1.pts;
        outFrame_num++;
        DecodeThread::audio_empty++;
        if(DecodeThread::audio_empty==5)
            DecodeThread::audio_noEmpty.wakeAll();
        DecodeThread::audio_mutex.unlock();
        if(!init_time)
        {
            //DecodeThread::start_time=(double)clock();
            DecodeThread::condition.wakeAll();
            init_time=true;
        }
    }

    if (len_written >= m_buffer1.len)
    {
        //if(DecodeThread::audio_outBuffer.empty())
        //return 0;
        //else
        {
            len_written=0;
            /*if(!m_buffer1.buffer)
                av_free(m_buffer1.buffer);
            DecodeThread::audio_mutex.lock();

            m_buffer1=DecodeThread::audio_outBuffer.dequeue();
            DecodeThread::audio_time=m_buffer1.pts;
            DecodeThread::audio_mutex.unlock();*/

            DecodeThread::audio_mutex.lock();
            DecodeThread::audio_full--;
            if(DecodeThread::audio_full<0)
            {
                if(DecodeThread::isFinsh)
                {
                    DecodeThread::audio_mutex.unlock();
                    emit isFinish();
                    return -1;
                }
                DecodeThread::audio_nofull.wait(&DecodeThread::audio_mutex);
            }
            m_buffer1.buffer=DecodeThread::audio_outdata[outFrame_num%DecodeThread::max_audioFrame].buffer;
            m_buffer1.len=DecodeThread::audio_outdata[outFrame_num%DecodeThread::max_audioFrame].len;
            m_buffer1.pts=DecodeThread::audio_outdata[outFrame_num%DecodeThread::max_audioFrame].pts;
            DecodeThread::audio_time=m_buffer1.pts;
            outFrame_num++;
            DecodeThread::audio_empty++;
            if(DecodeThread::audio_empty==5)
                DecodeThread::audio_noEmpty.wakeAll();
            DecodeThread::audio_mutex.unlock();
        }
    }

    /*if(!DecodeThread::video_outBuffer.empty())
    {
        double audio_clock=DecodeThread::video_outBuffer.front().pts;
        audio_clock+=static_cast<double>(len_written) /
                (2 * m_format.channelCount() * m_format.sampleRate());
        //获取上一帧需要显示的时长delay
        double delay=DecodeThread::video_outBuffer.front().pts-frame_last_pts;
        if (delay <= 0 || delay >= 1.0)
        {
            delay = frame_last_delay;
        }
        // 根据视频PTS和参考时钟调整delay
        // diff < 0 :video slow,diff > 0 :video fast
        double diff=DecodeThread::video_outBuffer.front().pts-audio_clock;
        delay=getDelay(audio_clock,delay,diff);
        frame_last_delay = delay;
        if(delay<=0.01)
        {
            frame_last_pts = DecodeThread::video_outBuffer.front().pts;
            qDebug()<<"audioplay ptr:"<<DecodeThread::video_outBuffer.front().buffer;
            emit sigGetOneFrame(DecodeThread::video_outBuffer.dequeue().buffer,delay);
        }





        //DecodeThread::video_mutex.lock();

        //DecodeThread::video_mutex.lock();





        //DecodeThread::video_mutex.unlock();
        //DecodeThread::video_num--;
        //DecodeThread::video_mutex.unlock();
        //if(DecodeThread::video_num<4)
        //DecodeThread::empty.wakeAll();

    }*/





    int lenth;

    //计算未播放的数据的长度.
    lenth = (len_written+len) > m_buffer1.len ? (m_buffer1.len - len_written) : len;


    memcpy(data, m_buffer1.buffer+len_written, lenth); //把要播放的pcm数据存入声卡缓冲区里.


     //qDebug()<<"audio:"<<DecodeThread::audio_time;
    //qDebug()<<"audio111111111111111111111:"<<outFrame_num;
    len_written += lenth; //更新已播放的数据长度.
    DecodeThread::audio_mutex.lock();
    DecodeThread::audio_time+=static_cast<double>(len_written) /
            (2 * m_format.channelCount() * m_format.sampleRate());
    DecodeThread::audio_mutex.unlock();

    return lenth;

}

qint64 MyDevice::writeData(const char *data, qint64 len)
{
    Q_UNUSED(data);
    Q_UNUSED(len);

    return 0;
}

qint64 MyDevice::bytesAvailable() const
{
    return m_buffer.size() + QIODevice::bytesAvailable();
}

double MyDevice::getDelay(double audio_clock,double delay,double diff)
{
    //一帧视频时间或10ms，10ms音视频差距无法察觉
    double sync_threshold = FFMAX(0.01, FFMIN(0.1, delay)) ;

    if (!isnan(diff) && fabs(diff) < NOSYNC_THRESHOLD) // 不同步
    {
        if (diff <= -sync_threshold)//视频比音频慢，加快
            delay = FFMAX(0,  delay + diff);
        //视频比音频快，减慢
        else if (diff >= sync_threshold && delay > 0.1)
            delay = delay + diff;//音视频差距较大，且一帧的超过帧最常时间，一步到位
        else if (diff >= sync_threshold)
            delay = 2 * delay;//音视频差距较小，加倍延迟，逐渐缩小
    }

    return delay;
}







AudioPlay::AudioPlay(QObject *parent)
    : QObject{parent}
{
    m_format.setSampleFormat(QAudioFormat::Int16);
    m_format.setChannelConfig(QAudioFormat::ChannelConfigStereo);
    m_format.setChannelCount(2);
    m_format.setSampleRate(44100);
    m_isstop=false;
    m_isstart=false;
    connect(this,SIGNAL(inited()),this,SLOT(playaudio()));
}

AudioPlay::AudioPlay(PlayWidget* playwidget):m_playwidget(playwidget)
{
    m_format.setSampleFormat(QAudioFormat::Int16);
    m_format.setChannelConfig(QAudioFormat::ChannelConfigStereo);
    m_format.setChannelCount(2);
    m_format.setSampleRate(44100);


}

AudioPlay::~AudioPlay()
{
    this->stop();
}

void AudioPlay::stop()
{
    if(!m_isstop)
    {
        m_format.setSampleRate(44100);
        if(m_isstart)
        {
            m_audioOutput->stop();
            m_mydevices->stop();
            delete m_mydevices;
            delete m_audioOutput;
            m_isstart=false;
        }

        m_isstop=true;
    }

}

void AudioPlay::init(int sampleRate)
{
    m_format.setSampleRate(sampleRate);
    //m_mydevices=new MyDevice(m_format,m_playwidget);
    m_mydevices=new MyDevice(m_format);
    m_audioOutput=new QAudioSink(m_devices->defaultAudioOutput(),m_format);
    //QObject::connect(m_mydevices,SIGNAL(isFinsh()),this,SLOT(stop()));
    m_isstop=false;
    emit inited();
}

void AudioPlay::init(int sampleRate,bool type)
{
    m_format.setSampleRate(sampleRate);
    //m_mydevices=new MyDevice(m_format,m_playwidget);
    m_mydevices=new MyDevice(m_format);
    m_audioOutput=new QAudioSink(m_devices->defaultAudioOutput(),m_format);
    //QObject::connect(m_mydevices,SIGNAL(isFinsh()),this,SLOT(stop()));
    if(type)
        m_audioOutput->setVolume(0);
    m_isstop=false;
    emit inited();
}


void AudioPlay::playaudio()
{
    qDebug()<<"AudioPlay run!";

    m_audioOutput->setBufferSize(50000*2);
    m_audioOutput->start(m_mydevices);
    m_isstart=true;
}

void AudioPlay::startplay()
{
    qDebug()<<"AudioPlay run!";
    m_audioOutput=new QAudioSink(m_devices->defaultAudioOutput(),m_format);
    m_audioOutput->start(m_mydevices);
    m_playwidget->show();
}
