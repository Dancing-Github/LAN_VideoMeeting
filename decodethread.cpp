#include "decodethread.h"
double DecodeThread::videoFPS;
uchar* DecodeThread::out_buffer=nullptr;
uchar* DecodeThread::audio_buffer;
QQueue<ABuffer> DecodeThread::video_outBuffer;
QQueue<ABuffer> DecodeThread::audio_outBuffer;
QMutex DecodeThread::video_mutex;
QMutex DecodeThread::audio_mutex;
QWaitCondition DecodeThread::condition;
QWaitCondition DecodeThread::exist;
QWaitCondition DecodeThread::enque;
int DecodeThread::audio_len=0;
int DecodeThread::MAX_AUDIO_SIZE;
int DecodeThread::video_num=0;
int DecodeThread::audio_num=0;
double DecodeThread::delay=0.0416667;
double DecodeThread::start_time=-1.0;
double DecodeThread::audio_time=-1.0;
uint DecodeThread::video_width=0.0;
uint DecodeThread::video_height=0.0;
bool DecodeThread::isFinsh=false;
int DecodeThread::max_videoFrame=30;
int DecodeThread::max_audioFrame=100;
int DecodeThread::max_vCapacity=50;
QVector<ABuffer> DecodeThread::video_outdata(max_videoFrame);
QVector<ABuffer> DecodeThread::audio_outdata(max_audioFrame);
int DecodeThread::video_full=0;
int DecodeThread::video_empty=max_videoFrame-10;
int DecodeThread::audio_full=0;
int DecodeThread::audio_empty=max_audioFrame-10;
QWaitCondition DecodeThread::video_nofull;
QWaitCondition DecodeThread::video_noEmpty;
QWaitCondition DecodeThread::audio_nofull;
QWaitCondition DecodeThread::audio_noEmpty;
uchar* DecodeThread::ptr;

int DecodeThread::decode_interrupt_cb(void *ctx)
{
    DecodeThread *pThis = (DecodeThread*)ctx;
    //microseconds 微秒
    int64_t nDurTime = av_gettime() - pThis->m_nStartOpenTS;
    if (nDurTime > 3000000 && pThis->m_nStartOpenTS>0)
    {
        return 1;
    }
    return 0;

}

DecodeThread::DecodeThread(QObject *parent)
    : QThread{parent}
{
    this->m_avstate=new AVState();
    this->init();
    //his->m_filePath="E:/VS_Documents/Ffmgeptest/test.mp4";
    //this->m_filePath="rtp://234.234.234.234:23334";
    //this->m_filePath="";

}

void DecodeThread::init()
{
    this->m_isstop=false;
    this->m_isAinit=false;
    this->m_isVinit=false;
    this->m_isdel=false;
    this->m_type=false;
    this->isFinsh=false;
    this->flag=false;
    this->flag1=false;


    video_clock=0.0;
    audio_clock=0.0;
    last_video_pts=0.0;
    m_nStartOpenTS=0;

    avdevice_register_all();
    m_avstate->av_format_context=avformat_alloc_context();
    m_avstate->av_format_context->interrupt_callback.callback = decode_interrupt_cb;
    m_avstate->av_format_context->interrupt_callback.opaque = this;
    m_avstate->av_packet=av_packet_alloc();

    DecodeThread::videoFPS=0;
    DecodeThread::audio_len=0;
    DecodeThread::video_num=0;
    DecodeThread::audio_num=0;
    DecodeThread::delay=0.0416667;
    DecodeThread::start_time=-1.0;
    DecodeThread::audio_time=-1.0;
    DecodeThread::video_width=0.0;
    DecodeThread::video_height=0.0;
    DecodeThread::isFinsh=false;


    DecodeThread::video_full=0;
    DecodeThread::video_empty=max_videoFrame-10;
    DecodeThread::audio_full=0;
    DecodeThread::audio_empty=max_audioFrame-10;
}

DecodeThread::~DecodeThread()
{
    del();
    delete m_avstate;

}

void DecodeThread::setFilePath(QString path)
{
    //qDebug()<<"path: "<<path;
    /*QByteArray ba = path.toUtf8();
    int len =strlen(ba.data());
    qDebug()<<"len:"<<len;
    memcpy(this->m_filePath,ba.data(),len);*/
    m_path=path;
    //qDebug()<<"filePath: "<<this->m_filePath;
}

void DecodeThread::setType()
{
    this->m_type=true;
}

void DecodeThread::run()
{


    qDebug()<<"startThread!";
    //qDebug()<<this->m_filePath;
    qDebug()<<"11111111111111111";
    int is_getdata=getData();
    if(is_getdata!=0)
    {
        char* error_info=new char[32];
        av_strerror(is_getdata,error_info,1024);
        qDebug()<<QString("error%1").arg(error_info);
        delete[] error_info;

        isFinsh=true;
        DecodeThread::exist.wakeAll();
        audio_nofull.wakeAll();
        emit happenFail();
        return;
    }
    m_nStartOpenTS=0;


    int is_getinfo=getInfo();
    if(is_getinfo<0)
    {
        //获取失败
        char* error_info=new char[32];
        av_strerror(is_getinfo,error_info,1024);
        qDebug()<<QString("error%1").arg(error_info);
        delete []  error_info;
        isFinsh=true;
        DecodeThread::exist.wakeAll();
        audio_nofull.wakeAll();
        emit happenFail();
        return;
    }

    findInfo();
    if(m_avstate->video_stream_id!=-1)
        initVideo();
    if(m_avstate->video_stream_id!=-1 && !m_isVinit)
    {
        isFinsh=true;
        DecodeThread::exist.wakeAll();
        audio_nofull.wakeAll();
        emit happenFail();
        return;
    }

    if(m_avstate->audio_stream_id!=-1)
        initAudio();

    while(av_read_frame(m_avstate->av_format_context,m_avstate->av_packet)>=0 && !m_isstop)
    {
        //解码什么类型流(视频流、音频流、字幕流等等...)
        if(m_avstate->av_packet->stream_index==m_avstate->video_stream_id)
        {
            m_avstate->video_stream=m_avstate->av_format_context->streams[m_avstate->av_packet->stream_index];
            //qDebug()<<"video!";
            decodeVideo();
        }

        if(m_avstate->av_packet->stream_index==m_avstate->audio_stream_id)
        {
            m_avstate->audio_stream=m_avstate->av_format_context->streams[m_avstate->av_packet->stream_index];
            //qDebug()<<"audio!";
            decodeAudio();
        }
    }

    isFinsh=true;
    DecodeThread::exist.wakeAll();
    audio_nofull.wakeAll();
    //del();
}

//本地文件
/*int DecodeThread::getData()
{
    int ret=avformat_open_input(&m_avstate->av_format_context,m_filePath,nullptr,nullptr);

    return ret;
}*/

//rtp 文件
int DecodeThread::getData()
{
    AVDictionary *d = NULL;
    av_dict_set(&d, "protocol_whitelist", "file,udp,rtp", 0);
    av_dict_set(&d, "rtsp_transport", "udp", 0);
    av_dict_set(&d,"probesize","40960000",0);
    av_dict_set(&d,"listen_timeout","3",0);
    //av_dict_set(&d,"timeout","3000000",0);
    //av_dict_set(&d,"stimeout","3000000",0);
    //av_dict_set(&d,"analyzeduration","300000",0);
    //av_dict_set(&d,"max_analyse_duration","10",0);

    QByteArray ba = m_path.toUtf8();
    int len =strlen(ba.data());
    qDebug()<<"len:"<<len;
    qDebug()<<"ba: "<<ba.data();
    m_nStartOpenTS = av_gettime();
    int ret=avformat_open_input(&m_avstate->av_format_context, ba.data(),nullptr,&d);
    qDebug()<<"ret"<<ret;
    return ret;
}

int DecodeThread::getInfo()
{
    //qDebug()<<"begin getInfo()!";
    int ret=avformat_find_stream_info(m_avstate->av_format_context,nullptr);
    return ret;
}

void DecodeThread::findInfo()
{
    for(int i=0;i<m_avstate->av_format_context->nb_streams;++i)
    {
        //循环遍历每一流
        //视频流、音频流、字幕流等等...
        if(m_avstate->av_format_context->streams[i]->codecpar->codec_type==AVMEDIA_TYPE_VIDEO)
        {
            //找到了视频流
            m_avstate->video_stream_id=i;
            DecodeThread::videoFPS =  m_avstate->av_format_context->streams[i]->avg_frame_rate.num/m_avstate->av_format_context->streams[i]->avg_frame_rate.den;
            //qDebug()<<"Receiving FPS"<<--videoFPS;
        }
        if(m_avstate->av_format_context->streams[i]->codecpar->codec_type==AVMEDIA_TYPE_AUDIO)
        {
            //找到了音频流
            m_avstate->audio_stream_id=i;
        }
    }
    if(m_avstate->video_stream_id==-1)
    {
        qDebug()<<QString("no find video_stream");
    }
    if(m_avstate->audio_stream_id==-1)
    {
        qDebug()<<QString("no find audio_stream");
    }

}

void DecodeThread::initVideo()
{
    m_avstate->video_codec_context=avcodec_alloc_context3(NULL);
    avcodec_parameters_to_context(m_avstate->video_codec_context,
                                  m_avstate->av_format_context->streams[m_avstate->video_stream_id]->codecpar);
    if(m_avstate->video_codec_context->width*m_avstate->video_codec_context->height==0)
        return;

    qDebug()<<"try to initVideo";

    for(int codecIndex=0;codecIndex<4;++codecIndex)
    {
        switch(codecIndex)
        {
        case 0:
            m_avstate->video_codec_context->codec=avcodec_find_decoder_by_name("hevc_cuvid") ;
            m_avstate->video_codec_context->pix_fmt = AV_PIX_FMT_YUV420P;
            break;
        case 1:
            m_avstate->video_codec_context->codec=avcodec_find_decoder_by_name("hevc_amf");
            m_avstate->video_codec_context->pix_fmt = AV_PIX_FMT_NV12;
            break;
        case 2:
            m_avstate->video_codec_context->codec=avcodec_find_decoder_by_name("hevc_qsv");
            m_avstate->video_codec_context->pix_fmt = AV_PIX_FMT_NV12;
            break;
        case 3:
            m_avstate->video_codec_context->codec=avcodec_find_decoder(m_avstate->video_codec_context->codec_id);
            m_avstate->video_codec_context->pix_fmt = AV_PIX_FMT_YUV420P;
            break;
        }

        if(! m_avstate->video_codec_context->codec)
        {
            if(codecIndex==3)
            {
                qDebug()<<"can not find the decoder!";
                return ;
            }
            continue;
        }

        if (avcodec_open2(m_avstate->video_codec_context,  m_avstate->video_codec_context->codec , NULL)==0)
        {
            qDebug()<<"successfully open the decoder!"<<codecIndex;
            qDebug()<<QString("file format： %1").arg(m_avstate->av_format_context->iformat->name);
            //输出：解码器名称
            qDebug()<<QString("decoder name： %1").arg( m_avstate->video_codec_context->codec->name);
            qDebug()<<QString("width %1 height %2").arg(m_avstate->video_codec_context->width).arg(m_avstate->video_codec_context->height);
            //此函数自动打印输入或输出的详细信息
            //av_dump_format(m_avstate->av_format_context, 0, m_filePath, 0);
            break;
        }
        else if(codecIndex==3)
        {
            qDebug()<<"cannot open the decoder!";
            return ;
        }
    }

    //输入->环境一帧数据->缓冲区->类似于一张图
    m_avstate->video_inframe=av_frame_alloc();
    //输出->帧数据->视频像素数据格式->yuv420p
    m_avstate->video_outframe=av_frame_alloc();
    //缓冲区分配内存
    m_avstate->video_outbuffer=(uint8_t*)av_malloc(av_image_get_buffer_size(AV_PIX_FMT_YUV420P,m_avstate->video_codec_context->width,m_avstate->video_codec_context->height,1));
    //初始化缓冲区
    av_image_fill_arrays(m_avstate->video_outframe->data,
                         m_avstate->video_outframe->linesize,
                         m_avstate->video_outbuffer,AV_PIX_FMT_YUV420P,
                         m_avstate->video_codec_context->width,
                         m_avstate->video_codec_context->height,1);
    if(!flag)
    {
        out_buffer=(uint8_t*)av_malloc(av_image_get_buffer_size(AV_PIX_FMT_YUV420P,m_avstate->video_codec_context->width,m_avstate->video_codec_context->height,1));
        av_image_fill_arrays(m_avstate->video_outframe->data,
                             m_avstate->video_outframe->linesize,
                             out_buffer,AV_PIX_FMT_YUV420P,
                             m_avstate->video_codec_context->width,
                             m_avstate->video_codec_context->height,1);

        flag=true;
    }

    //qDebug()<<"receive pix fmt: "<<AV_PIX_FMT_YUV420P <<"        "<<m_avstate->av_format_context->streams[m_avstate->video_stream_id]->codecpar->format;

    m_avstate->sws_context=sws_getContext(m_avstate->video_codec_context->width,
                                          m_avstate->video_codec_context->height,
                                          m_avstate->video_codec_context->pix_fmt,
                                          m_avstate->video_codec_context->width,
                                          m_avstate->video_codec_context->height,
                                          AV_PIX_FMT_YUV420P,
                                          SWS_BICUBIC,NULL,NULL,NULL);

    /*m_video_outfile=fopen("test3.yuv","wb+");
    if(!m_video_outfile)
    {
        qDebug()<<"cann't open file ";
    }*/
    video_width=m_avstate->video_codec_context->width;
    video_height=m_avstate->video_codec_context->height;
    //emit sigGetWH(m_avstate->video_codec_context->width,m_avstate->video_codec_context->height);
    m_isVinit=true;
}

void DecodeThread::initAudio()
{
    m_avstate->audio_codec_context=avcodec_alloc_context3(NULL);
    avcodec_parameters_to_context(m_avstate->audio_codec_context,
                                  m_avstate->av_format_context->streams[m_avstate->audio_stream_id]->codecpar);
    const AVCodec* audio_avcodec=avcodec_find_decoder( m_avstate->audio_codec_context->codec_id);
    if(!audio_avcodec)
    {
        qDebug()<<QString("no find audio decoder");
    }

    int audio_avcodec_open2_result=avcodec_open2(m_avstate->audio_codec_context,
                                                 audio_avcodec,NULL);
    if(audio_avcodec_open2_result!=0)
    {
        char* error_info=new char[32];
        av_strerror(audio_avcodec_open2_result,error_info,1024);
        qDebug()<<QString("异常信息%1").arg(error_info);
    }

    qDebug()<<QString("file format： %1").arg(m_avstate->av_format_context->iformat->name);
    //输出：解码器名称
    qDebug()<<QString("audio decoder name: %1").arg(audio_avcodec->name);
    //此函数自动打印输入或输出的详细信息
    //av_dump_format(m_avstate->av_format_context, 0, m_filePath, 0);

    //创建音频采样数据帧
    m_avstate->audio_inframe=av_frame_alloc();

    // 确保解码为pcm我们需要转换为pcm格式
    m_avstate->swr_context=swr_alloc();
    m_avstate->swr_context=swr_alloc_set_opts(m_avstate->swr_context,
                                              AV_CH_LAYOUT_STEREO,
                                              AV_SAMPLE_FMT_S16,
                                              m_avstate->audio_codec_context->sample_rate,
                                              m_avstate->audio_codec_context->channel_layout,
                                              m_avstate->audio_codec_context->sample_fmt,
                                              m_avstate->audio_codec_context->sample_rate,
                                              NULL,NULL);
    qDebug()<<"sample_rate:"<<m_avstate->audio_codec_context->sample_rate;
    //初始化音频采样数据上下文
    swr_init(m_avstate->swr_context);
    //缓冲区大小 = 采样率(44100HZ) * 采样精度(16位 = 2字节)
    MAX_AUDIO_SIZE=44100*2;
    m_avstate->audio_outbuffer=(uint8_t*)av_malloc(MAX_AUDIO_SIZE);
    if(!flag1)
    {
        audio_buffer=(uint8_t*)av_malloc(MAX_AUDIO_SIZE);
        for(int i=0;i<max_audioFrame;++i)
            audio_outdata[i].buffer=(uint8_t*)av_malloc(MAX_AUDIO_SIZE);
        flag1=true;
    }

    //获取输出的声道个数
    m_avstate->out_nb_channels=av_get_channel_layout_nb_channels(AV_CH_LAYOUT_STEREO);


    /*m_audio_outfile = fopen("test3.pcm","wb+");
    if (!m_audio_outfile)
    {
        qDebug()<<QString("文件不存在 ");
    }*/
    emit sigGetSampleRate(m_avstate->audio_codec_context->sample_rate,m_type);

    m_isAinit=true;
}

void DecodeThread::decodeVideo()
{
    //qDebug()<<"begin decodeVideo()!";
    //发送一帧数据
    avcodec_send_packet(m_avstate->video_codec_context,m_avstate->av_packet);
    //接收一帧数据->解码一帧

    //解码出来的每一帧数据成功之后，将每一帧数据保存为YUV420P格式文件类型(.yuv文件格式)

    if(avcodec_receive_frame(m_avstate->video_codec_context,m_avstate->video_inframe)==0)
    {
        int ret =sws_scale(m_avstate->sws_context,
                           m_avstate->video_inframe->data,
                           m_avstate->video_inframe->linesize,0,
                           m_avstate->video_codec_context->height,
                           m_avstate->video_outframe->data,
                           m_avstate->video_outframe->linesize);
        if(ret<0)
            qDebug()<<"display scaler"<<ret;
        //
        //yuv420规则一：Y结构表示一个像素点
        //yuv420规则二：四个Y对应一个U和一个V（也就是四个像素点，对应一个U和一个V）
        // y = 宽 * 高
        // u = y / 4
        // v = y / 4
        /*m_avstate->y_size=m_avstate->video_codec_context->width*m_avstate->video_codec_context->height;
        m_avstate->u_size=m_avstate->y_size/4;
        m_avstate->v_size=m_avstate->y_size/4;

        //写入->Y
        //av_frame_in->data[0]:表示Y
        fwrite(m_avstate->video_outframe->data[0],1,m_avstate->y_size,m_video_outfile);
        //写入->U
        //av_frame_in->data[1]:表示U
        fwrite(m_avstate->video_outframe->data[1], 1,m_avstate->u_size, m_video_outfile);
        //写入->V8/
        //av_frame_in->data[2]:表示V
        fwrite(m_avstate->video_outframe->data[2], 1,m_avstate->v_size, m_video_outfile);*/

        /*QImage tmpimage((uchar*)m_avstate->video_outbuffer,
                        m_avstate->video_codec_context->width,
                        m_avstate->video_codec_context->height,
                        QImage::Format_RGB32);
        QImage image=tmpimage.copy();
        emit sigGetOneFrame(image);*/

        /*QTime dieTime = QTime::currentTime().addMSecs(30);
        while( QTime::currentTime() < dieTime );*/

        ABuffer tmp_buffer;
        tmp_buffer.buffer=(uint8_t*)av_malloc(av_image_get_buffer_size(AV_PIX_FMT_YUV420P,m_avstate->video_codec_context->width,m_avstate->video_codec_context->height,1));
        av_image_fill_arrays(m_avstate->video_outframe->data,
                             m_avstate->video_outframe->linesize,
                             tmp_buffer.buffer,AV_PIX_FMT_YUV420P,
                             m_avstate->video_codec_context->width,
                             m_avstate->video_codec_context->height,1);

        if ((tmp_buffer.pts = m_avstate->video_inframe->pts) == AV_NOPTS_VALUE)
        {
            tmp_buffer.pts = 0;
        }


        tmp_buffer.pts*=av_q2d(m_avstate->video_stream->time_base);
        tmp_buffer.pts=synchronize(m_avstate->video_inframe,tmp_buffer.pts);
        //qDebug()<<"video_clock:"<<tmp_buffer.pts;



        int bytes =0;
        for(int i=0;i<m_avstate->video_codec_context->height;i++){
            memcpy(out_buffer+bytes,
                   m_avstate->video_outframe->data[0]+m_avstate->video_outframe->linesize[0]*i,
                    m_avstate->video_codec_context->width);
            bytes+=m_avstate->video_codec_context->width;
        }

        int u=m_avstate->video_codec_context->height>>1;
        for(int i=0;i<u;i++){
            memcpy(out_buffer+bytes,
                   m_avstate->video_outframe->data[1]+m_avstate->video_outframe->linesize[1]*i,
                    m_avstate->video_codec_context->width/2);
            bytes+=m_avstate->video_codec_context->width/2;
        }

        for(int i=0;i<u;i++){
            memcpy(out_buffer+bytes,
                   m_avstate->video_outframe->data[2]+m_avstate->video_outframe->linesize[2]*i,
                    m_avstate->video_codec_context->width/2);
            bytes+=m_avstate->video_codec_context->width/2;
        }

        //tmp_buffer.buffer=(uint8_t*)av_malloc(bytes);
        memcpy(tmp_buffer.buffer,out_buffer,bytes);



        video_mutex.lock();
        if(video_outBuffer.size()>=max_vCapacity)
            enque.wait(&video_mutex);
        video_outBuffer.enqueue(tmp_buffer);
        video_mutex.unlock();
        video_num++;
        //qDebug()<<"video:"<<video_num;


        DecodeThread::exist.wakeAll();

    }
}

void DecodeThread::decodeAudio()
{
    //qDebug()<<"begin decodeAudio()!";
    int ret;
    //音频解码
    //1、发送一帧音频压缩数据包->音频压缩数据帧
    avcodec_send_packet(m_avstate->audio_codec_context,m_avstate->av_packet);
    if (m_avstate->av_packet->pts != AV_NOPTS_VALUE)
    {
        audio_clock = av_q2d(m_avstate->audio_stream->time_base) * m_avstate->av_packet->pts;
        //qDebug()<<"audio_clock:"<<audio_clock;
    }
    //2、解码一帧音频压缩数据包->得到->一帧音频采样数据->音频采样数据帧
    while((ret=avcodec_receive_frame(m_avstate->audio_codec_context,m_avstate->audio_inframe))>=0)
    {
        // 解码成功
        if(ret==0)
        {


            //获取采样数
            int nb_samples=m_avstate->audio_inframe->nb_samples;
            //获取每个采样点的大小
            int numBytes=av_get_bytes_per_sample(AV_SAMPLE_FMT_S16);
            // 修改采样率参数后，需要重新获取采样点的样本个数
            int out_nb_samples=av_rescale_rnd(m_avstate->audio_inframe->nb_samples,
                                              m_avstate->audio_inframe->sample_rate,
                                              m_avstate->audio_inframe->sample_rate,
                                              AV_ROUND_ZERO);
            //获取缓冲区实际存储大小
            int out_buffer_size=av_samples_get_buffer_size(NULL,
                                                           m_avstate->out_nb_channels,
                                                           m_avstate->audio_inframe->nb_samples,
                                                           AV_SAMPLE_FMT_S16,
                                                           1);
            //AVFrame* audio_out_avframe=av_frame_alloc();

            /*audio_out_avframe->nb_samples=out_nb_samples;
            audio_out_avframe->channels=out_nb_channels;
            audio_out_avframe->channel_layout=AV_CH_LAYOUT_STEREO;
            audio_out_avframe->format=AV_SAMPLE_FMT_S16;
            audio_out_avframe->sample_rate=audio_context->sample_rate;*/
            //av_frame_get_buffer(audio_out_avframe,0);
            /*av_samples_fill_arrays(audio_out_avframe->data,
               audio_out_avframe->linesize,
               out_audio,
               out_nb_channels,
               out_nb_samples,
               AV_SAMPLE_FMT_S16,
               1);*/



            //4、类型转换(音频采样数据格式有很多种类型)    重采样
            //我希望我们的音频采样数据格式->pcm格式->保证格式统一->输出PCM格式文件
            //swr_convert:表示音频采样数据类型格式转换器
            //参数一：音频采样数据上下文
            //参数二：输出音频采样数据
            //参数三：输出音频采样数据->大小
            //参数四：输入音频采样数据
            //参数五：输入音频采样数据->大小
            swr_convert(m_avstate->swr_context,
                        &m_avstate->audio_outbuffer,
                        out_nb_samples,
                        (const uint8_t**)m_avstate->audio_inframe->data,
                        m_avstate->audio_inframe->nb_samples);
            //ABuffer tmp_buffer;
            //tmp_buffer.len=out_buffer_size;
            //tmp_buffer.buffer=(uint8_t*)av_malloc(MAX_AUDIO_SIZE);
            //memcpy(tmp_buffer.buffer, (char*)m_avstate->audio_outbuffer, out_buffer_size);
            //tmp_buffer.pts=audio_clock;




            audio_mutex.lock();
            audio_empty--;
            if(audio_empty<0)
                audio_noEmpty.wait(&audio_mutex);
            audio_outdata[audio_num%max_audioFrame].len=out_buffer_size;
            audio_outdata[audio_num%max_audioFrame].pts=audio_clock;
            memcpy(audio_outdata[audio_num%max_audioFrame].buffer, (char*)m_avstate->audio_outbuffer, out_buffer_size);
            audio_num++;
            audio_full++;
            if(audio_full==1)
                audio_nofull.wakeAll();
            audio_mutex.unlock();
            //qDebug()<<"audio:"<<audio_num;


            // 每秒钟音频播放的字节数 sample_rate * channels * sample_format(一个sample占用的字节数)
            audio_clock += static_cast<double>(out_buffer_size) / (2 * m_avstate->out_nb_channels *
                                                                   m_avstate->audio_inframe->sample_rate);

            //audio_outBuffer.enqueue(tmp_buffer);

            //fwrite((char*)m_avstate->audio_outbuffer,out_buffer_size,1,m_audio_outfile);

        }
    }
}

void DecodeThread::del()
{
    if(!m_isdel)
    {
        if(m_isVinit)
        {
            //fclose(m_video_outfile);
            av_frame_free(&m_avstate->video_inframe);
            av_frame_free(&m_avstate->video_outframe);
            av_free(m_avstate->video_outbuffer);
            sws_freeContext(m_avstate->sws_context);
            avcodec_close(m_avstate->video_codec_context);
        }
        if(m_isAinit)
        {
            //fclose(m_audio_outfile);
            av_frame_free(&m_avstate->audio_inframe);
            av_free(m_avstate->audio_outbuffer);
            swr_free(&m_avstate->swr_context);
            avcodec_close(m_avstate->audio_codec_context);
        }
        if(!isFinsh)
        {
            av_packet_free(&m_avstate->av_packet);
            avformat_free_context(m_avstate->av_format_context);
        }

        //delete m_avstate;
        isFinsh=true;
        m_isdel=true;
    }

}

void DecodeThread::stop()
{
    m_isstop=true;
    //del();
}

double DecodeThread::synchronize(AVFrame *inFrame,double pts)
{
    double frame_delay;

    if (pts != 0)
        video_clock = pts; // Get pts,then set video clock to it
    else
        pts = video_clock; // Don't get pts,set it to video clock

    frame_delay = av_q2d(m_avstate->av_packet->time_base);
    frame_delay += inFrame->repeat_pict * (frame_delay * 0.5);

    video_clock += frame_delay;

    return pts;
}

