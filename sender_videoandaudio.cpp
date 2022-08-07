#include "sender_videoandaudio.h"
#include<QTimer>
#include <QtConcurrent>
#include <QThread>
#include<QScreen>
#include<QMessageBox>
#include<QDebug>

/*
void my_logoutput(void* ptr, int level, const char* fmt, va_list vl)
{
    qDebug()<<QString(fmt)<<vl;
}
*/

sender_VideoAndAudio::sender_VideoAndAudio(QString audioDeviceName, QString rtpOutputAddress, QWidget *parent)
    : audioDevice(audioDeviceName), outputAddress(rtpOutputAddress), QOpenGLWidget{parent}
{   
    start=QTime::currentTime();//用于计算音视频的微小偏差

    fps=15;//设置输出fps，输入输出建议相同
    bCap=false;
    outFileOpened=false;
    avdevice_register_all();//初始化所有设备
    //av_log_set_level(AV_LOG_DEBUG);

    qDebug()<<"##############错误码#############\n"
           <<AVERROR(EAGAIN)<<"AVERROR(EAGAIN)\n"<<
             AVERROR_EOF<<"AVERROR_EOF\n"<<
             AVERROR(EINVAL)<<" AVERROR(EINVAL)\n";


    if(!openVideoCapture())
    {
        qDebug()<<"###############V视频录制未打开############";
        //emit error("视频录制未打开");
        return  ;
    }
    if(!openAudioCapture())
    {
        qDebug()<<"###############A音频录制未打开############";
        //emit  error("音频录制未打开");
        return ;
    }
    if(!openOutPut())
    {
        qDebug()<<"##############O未打开输出通道#############";
        //emit   error("未打开输出通道");
        return ;
    }


    bCap = true;
    //star cap audio thread
    AudioProc=QtConcurrent::run([this] {return AudioCapThreadProc();});
    //star cap screen thread
    ScreenProc=QtConcurrent::run([this] {return ScreenCapThreadProc();});
    //star WRITE thread
    WriteProc=QtConcurrent::run([this] {return  WritePacketThreadProc();});
    //star ENCODE audio thread
    EncodeProc=QtConcurrent::run([this] {return EncodeThreadProc();});

    /*QTimer* timer=new QTimer(this);
    connect(timer,&QTimer::timeout,this,&sender_VideoAndAudio::sendPaintEvent);
    timer->setInterval(1000);
    timer->start();*/
}

sender_VideoAndAudio::~sender_VideoAndAudio()
{
    //fclose(m_video_outfile);
    bCap = false;

    qDebug()<<"析构";

    AudioProc.waitForFinished();
    ScreenProc.waitForFinished();
    WriteProc.waitForFinished();
    EncodeProc.waitForFinished();

    qDebug()<<"析构1";
    if (pFormatCtx_inVideo)
    {
        avformat_close_input(&pFormatCtx_inVideo);
        //pFormatCtx_Video = nullptr;
    }
    if (pFormatCtx_inAudio)
    {
        avformat_close_input(&pFormatCtx_inAudio);
        //pFormatCtx_Audio = nullptr;
    }

    QThread::msleep(1500);
    qDebug()<<"析构2";
    av_fifo_free(fifo_video);
    av_fifo_free(fifo_local);
    av_audio_fifo_free(fifo_audio);

    //QThread::msleep(1000);
    qDebug()<<"析构3";
    av_free(localPictureBuffer);
    if(pFormatCtx_Out)
    {
        //此处析构判断
        if( outFileOpened)
        {
            av_write_trailer(pFormatCtx_Out);
            avio_close(pFormatCtx_Out->pb);
            outFileOpened=false;
        }
        //pFormatCtx_Out=nullptr;
    }

    qDebug()<<"析构4";
    avcodec_free_context(&pCodecCtx_inVideo);
    avcodec_free_context(&pCodecCtx_inAudio);
    avcodec_free_context(&pCodecCtx_outVideo);
    avcodec_free_context(&pCodecCtx_outAudio);
    avcodec_parameters_free(&pCodecPra_Out);

    qDebug()<<"析构5";
    avformat_free_context(pFormatCtx_Out);
    avformat_free_context(pFormatCtx_inVideo);
    avformat_free_context(pFormatCtx_inAudio);

    qDebug()<<"析构6";
    sws_freeContext(imgConvertContext);
    sws_freeContext(videoConvertContext);
    swr_free(&audioConvertContext);
    //还要有很多析构
    qDebug()<<"析构完成";
}

bool sender_VideoAndAudio::sender_Initialize()
{
    return bCap;
}


QString sender_VideoAndAudio::getResolution( int screenIndex)
{

    DISPLAY_DEVICEW device;
    device.cb = sizeof(device);
    EnumDisplayDevicesW(NULL, screenIndex, &device, 0);

    DEVMODEW device_mode;
    device_mode.dmSize = sizeof(device_mode);
    device_mode.dmDriverExtra = 0;
    EnumDisplaySettingsExW(device.DeviceName, ENUM_CURRENT_SETTINGS, &device_mode, 0);

    //    int x = device_mode.dmPosition.x;
    //    int y = device_mode.dmPosition.y;
    //    int width = device_mode.dmPelsWidth;
    //    int height = device_mode.dmPelsHeight;
    //    获取到物理分辨率，而非逻辑分辨率
    QString result=QString::number(device_mode.dmPelsWidth)+"x"+QString::number(device_mode.dmPelsHeight);
    return result;
}

bool sender_VideoAndAudio::openVideoCapture()
{
    pFormatCtx_inVideo=avformat_alloc_context();
    const AVInputFormat* inputFormat=av_find_input_format("gdigrab");

    //const  char* resolution=getResolution(0).toUtf8().toStdString().c_str();//获取主屏幕的物理分辨率
    //const  char* fpsInAndOut=QString(QString::number(fps)).toStdString().c_str();

    QByteArray resolution = getResolution(0).toUtf8();
    qDebug()<<"resolution: "<<resolution.data();

    QByteArray fpsInAndOut = QString(QString::number(fps)).toUtf8();
    qDebug()<<"fpsInAndOut: "<<fpsInAndOut.data();

    //以录制屏幕为输入
    AVDictionary* options=NULL;
    av_dict_set(&options,"framerate", fpsInAndOut.data(),0);
    av_dict_set(&options,"offset_x","0",0);
    av_dict_set(&options,"offset_y","0",0);
    av_dict_set(&options,"video_size", resolution.data(),0);
    //av_dict_set(&options,"video_size",resolution,0);
    av_dict_set(&options, "probesize","40960000",0);

    if(avformat_open_input(&pFormatCtx_inVideo,"desktop",inputFormat,&options))
    {
        qDebug()<<"找不到视频input流";
        return false;
    }

    if(avformat_find_stream_info(pFormatCtx_inVideo,NULL))
    {
        qDebug()<<"找不到视频stream imformation";
        return false;
    }

    videoIndex=-1;
    for(uint i=0;i<pFormatCtx_inVideo->nb_streams;++i)
    {
        if(pFormatCtx_inVideo->streams[i]->codecpar->codec_type==AVMEDIA_TYPE_VIDEO)
        {
            videoIndex=i;
            break;
        }
    }
    if(videoIndex==-1)
    {
        qDebug()<<"can not find video stream\n";
        return false;
    }
    //nbstream是流数量，一个语音视频一般两个
    //stream就是一个流的轨道，是指针数组
    pCodecPra_inVideo=pFormatCtx_inVideo->streams[videoIndex]->codecpar;
    pCodecCtx_inVideo=avcodec_alloc_context3(NULL);
    avcodec_parameters_to_context(pCodecCtx_inVideo,pCodecPra_inVideo);
    pCodecCtx_inVideo->codec=avcodec_find_decoder(pCodecPra_inVideo->codec_id);

    if(pCodecCtx_inVideo->codec==NULL)
    {
        qDebug()<<"cant find codec";
        return false;
    }

    //qDebug()<<"V"<< avcodec_get_name(pCodecPra_Video->codec_id);
    //codeccontext_Video->thread_count = 2;//多线程解码
    //codeccontext_Video->thread_type = FF_THREAD_FRAME;
    //codeccontext_Video->thread_type = FF_THREAD_SLICE;

    if(avcodec_open2(pCodecCtx_inVideo,pCodecCtx_inVideo->codec,NULL))
    {
        qDebug()<<"cant open codec";
        return false;
    }

    //qDebug()<<codeccontext_Video->thread_count;//线程数

    localVideoFrame=av_frame_alloc();
    YUVframe=av_frame_alloc();

    imgConvertContext=sws_getContext(pCodecPra_inVideo->width,pCodecPra_inVideo->height,pCodecCtx_inVideo->pix_fmt,pCodecPra_inVideo->width,pCodecPra_inVideo->height,AV_PIX_FMT_YUV420P,SWS_BICUBIC,nullptr,nullptr,nullptr);
    //本机显示图像的缓存
    localBufferSize=av_image_get_buffer_size(pCodecCtx_inVideo->pix_fmt,pCodecCtx_inVideo->width,pCodecCtx_inVideo->height,1);
    fifo_local=av_fifo_alloc( 2*  localBufferSize);
    localPictureBuffer=(unsigned char*)av_malloc((size_t)localBufferSize);

    /*
    m_video_outfile=fopen("test3.yuv","wb+");
    if(!m_video_outfile)
    {
        qDebug()<<"cann't open file ";
    }

*/


    return true;
}

bool sender_VideoAndAudio::openAudioCapture()
{
    pFormatCtx_inAudio=avformat_alloc_context();

    //查找输入方式
    const AVInputFormat* inputFormat = av_find_input_format("dshow");

    //以Direct Show的方式打开设备，并将 输入方式 关联到格式上下文
    //const char *psDevName =QString("audio="+audioDevice).toUtf8().toStdString().c_str();;//"audio=立体声混音 (Realtek(R) Audio)"

    QString name="audio="+audioDevice;
qDebug()<<"name"<<name;
    QByteArray psDevName =name.toUtf8();
    qDebug()<<"psDevName: "<<psDevName.data();

    if (avformat_open_input(&pFormatCtx_inAudio, psDevName.data(), inputFormat,NULL))
    {
        qDebug()<<"找不到音频input流";
        return false;
    }

    if(avformat_find_stream_info(pFormatCtx_inAudio,NULL))
    {
        qDebug()<<"找不到cstream imformation";
        return false;
    }

    audioIndex=-1;
    for(uint i=0;i<pFormatCtx_inAudio->nb_streams;++i)
    {
        if(pFormatCtx_inAudio->streams[i]->codecpar->codec_type==AVMEDIA_TYPE_AUDIO)
        {
            audioIndex=i;
            break;
        }
    }
    if(audioIndex==-1)
    {
        qDebug()<<"can not find audio stream（无法打开音频输入流）\n";
        return false;
    }

    //nbstream是流数量，一个语音视频一般两个
    //stream就是一个流的轨道，是指针数组
    pCodecPra_inAudio=pFormatCtx_inAudio->streams[audioIndex]->codecpar;
    if(pCodecPra_inAudio->channel_layout == 0)
    {
        pFormatCtx_inAudio->streams[audioIndex]->codecpar->channel_layout =av_get_default_channel_layout(pFormatCtx_inAudio->streams[audioIndex]->codecpar->channels);
        pCodecPra_inAudio->channel_layout =pFormatCtx_inAudio->streams[audioIndex]->codecpar->channel_layout;
    }
    else if(pCodecPra_inAudio->channels ==0)
    {
        pFormatCtx_inAudio->streams[audioIndex]->codecpar->channels=av_get_channel_layout_nb_channels(pFormatCtx_inAudio->streams[audioIndex]->codecpar->channel_layout);
        pCodecPra_inAudio->channels = pFormatCtx_inAudio->streams[audioIndex]->codecpar->channels;
    }

    pCodecCtx_inAudio=avcodec_alloc_context3(NULL);
    avcodec_parameters_to_context(pCodecCtx_inAudio,pCodecPra_inAudio);
    pCodecCtx_inAudio->codec=avcodec_find_decoder(pCodecPra_inAudio->codec_id);

    if(pCodecCtx_inAudio->codec==NULL)
    {
        qDebug()<<"cant find codec";
        return false;
    }

    qDebug()<<"A"<< avcodec_get_name(pCodecPra_inAudio->codec_id);

    if(avcodec_open2(pCodecCtx_inAudio,pCodecCtx_inAudio->codec,NULL))
    {
        qDebug()<<"cant open codec";
        return false;
    }

    return true;
}

bool sender_VideoAndAudio::openOutPut()
{
    //const char *outFileName = "rtp://234.234.234.234:23334";
    //const char *outFileName = "rtp://[FF05::10]:23333";
    //avformat_alloc_output_context2(&pFormatCtx_Out, NULL, "rtp_mpegts", outFileName);
    //const char *outFileName = outputAddress.toUtf8().toStdString().c_str();

    QByteArray outFileName = outputAddress.toUtf8();
    qDebug()<<"outFileName: "<<outFileName.data();

    qDebug()<<avformat_alloc_output_context2(&pFormatCtx_Out, NULL, "rtp_mpegts", outFileName.data());
    //const char *outFileName = "test.mp4";
    //avformat_alloc_output_context2(&pFormatCtx_Out, NULL, NULL, outFileName);

    if (pFormatCtx_inVideo->streams[videoIndex]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO)
    {
        pCodecCtx_outVideo=avcodec_alloc_context3(NULL);
        //pVideoStream = avformat_new_stream(pFormatCtx_Out, NULL);

        if (!avformat_new_stream(pFormatCtx_Out, NULL))//创建视频流0
        {
            qDebug()<<"can not new stream for output!\n";
            return false;
        }

        for(codecIndex=0;codecIndex<4;++codecIndex)
        {

            switch(codecIndex)
            {
            case 0:
                pCodecCtx_outVideo->codec=avcodec_find_encoder_by_name("hevc_nvenc") ;
                pCodecCtx_outVideo->pix_fmt= AV_PIX_FMT_YUV420P;
                break;
            case 1:
                pCodecCtx_outVideo->codec=avcodec_find_encoder_by_name("hevc_amf");
                pCodecCtx_outVideo->pix_fmt = AV_PIX_FMT_NV12;
                break;
            case 2:
                pCodecCtx_outVideo->codec=avcodec_find_encoder_by_name("hevc_qsv");
                pCodecCtx_outVideo->pix_fmt = AV_PIX_FMT_NV12;
                break;
            case 3:
                pCodecCtx_outVideo->codec=avcodec_find_encoder(AVCodecID::AV_CODEC_ID_HEVC);
                pCodecCtx_outVideo->pix_fmt= AV_PIX_FMT_YUV420P;
                break;
            }

            if( !pCodecCtx_outVideo->codec)
            {
                if(codecIndex==3)
                {
                    qDebug()<<"can not find the encoder!";
                    return false;
                }
                continue;
            }
            pCodecCtx_outVideo->height = pFormatCtx_inVideo->streams[videoIndex]->codecpar->height;
            pCodecCtx_outVideo->width = pFormatCtx_inVideo->streams[videoIndex]->codecpar->width;
            pCodecCtx_outVideo->time_base=AVRational{1,fps};
            pCodecCtx_outVideo->bit_rate = 3000000;
            pCodecCtx_outVideo->bit_rate_tolerance = 10;
            pCodecCtx_outVideo->gop_size=10;
            pCodecCtx_outVideo->qmin = 10;
            pCodecCtx_outVideo->qmax = 51;
            pCodecCtx_outVideo->sample_aspect_ratio = pFormatCtx_inVideo->streams[videoIndex]->codecpar->sample_aspect_ratio;

            if (pFormatCtx_Out->oformat->flags & AVFMT_GLOBALHEADER)
                pCodecCtx_outVideo->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

            AVDictionary* options=NULL;

            if( codecIndex==3) //软解专用
            {
                av_dict_set(&options, "x265-params", "bitrate=4000000", 0);
                av_dict_set(&options, "preset", "fast", 0);
                av_dict_set(&options, "tune", "zero-latency", 0);

                // take first format from list of supported formats
                //videoCodecCtx->pix_fmt = videoCodecCtx->codec->pix_fmts[0];
                pCodecCtx_outVideo->rc_max_rate = 4000000;
                pCodecCtx_outVideo->rc_min_rate = 4000000;
                pCodecCtx_outVideo->rc_buffer_size=2000000;
                pCodecCtx_outVideo->thread_count = 2;//多线程编码
                //videoCodecCtx->thread_type = FF_THREAD_FRAME;
                pCodecCtx_outVideo->thread_type = FF_THREAD_SLICE;
                QMessageBox::critical(this , "警告 " ,"即将使用CPU编码，效率较低" , QMessageBox::Ok | QMessageBox::Default | QMessageBox::Escape  ,NULL, NULL );

            }

            if (avcodec_open2(pCodecCtx_outVideo, pCodecCtx_outVideo->codec, &options)==0)
            {
                qDebug()<<"successfully open the encoder!"<<codecIndex;
                qDebug()<<"thread count"<<pCodecCtx_outVideo->thread_count;//线程数
                qDebug()<<"pix_fmt sent:"<<pCodecCtx_outVideo->pix_fmt;
                break;
            }
            else if(codecIndex==3)
            {
                qDebug()<<"cannot open the encoder!";
                return false;
            }
        }

        //将AVCodecContext的成员复制到AVCodecParameters结构体
        avcodec_parameters_from_context(pFormatCtx_Out->streams[0]->codecpar, pCodecCtx_outVideo);
        qDebug()<<"stream pix SPACE"<<pFormatCtx_Out->streams[0]->codecpar->format ;
    }

    if(pFormatCtx_inAudio->streams[audioIndex]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO)
    {
        pCodecCtx_outAudio=avcodec_alloc_context3(NULL);

        if (!avformat_new_stream(pFormatCtx_Out, NULL))//创建音频流1
        {
            qDebug()<<"can not new stream for output!";
            return false;
        }

        pCodecCtx_outAudio->codec= avcodec_find_encoder(AVCodecID::AV_CODEC_ID_AAC);
        pCodecCtx_outAudio->sample_rate = pFormatCtx_inAudio->streams[audioIndex]->codecpar->sample_rate;
        //qDebug()<<audioCodecCtx->sample_rate ;
        pCodecCtx_outAudio->bit_rate=192000;
        pCodecCtx_outAudio->bit_rate_tolerance=10;
        pCodecCtx_outAudio->channel_layout = pFormatCtx_inAudio->streams[audioIndex]->codecpar->channel_layout;
        pCodecCtx_outAudio->channels = pFormatCtx_inAudio->streams[audioIndex]->codecpar->channels;

        // qDebug()<<audioCodecCtx->channels<<" "<<audioCodecCtx->channel_layout;
        if(pCodecCtx_outAudio->channel_layout == 0)
            pCodecCtx_outAudio->channel_layout =pFormatCtx_inAudio->streams[audioIndex]->codecpar->channel_layout;
        else if(pCodecCtx_outAudio->channels ==0)
            pCodecCtx_outAudio->channels = pFormatCtx_inAudio->streams[audioIndex]->codecpar->channels;

        pCodecCtx_outAudio->time_base= AVRational{1, pCodecCtx_outAudio->sample_rate};
        // take first format from list of supported formats
        pCodecCtx_outAudio->sample_fmt = pCodecCtx_outAudio->codec->sample_fmts[0];
        //qDebug()<<"SAMPLE SPACE"<<audioCodecCtx->pix_fmt ;

        if (!pCodecCtx_outAudio->codec)
        {
            qDebug()<<"can not find the encoder!\n";
            return false;
        }

        if (pFormatCtx_Out->oformat->flags & AVFMT_GLOBALHEADER)
            pCodecCtx_outAudio->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

        if (avcodec_open2(pCodecCtx_outAudio, pCodecCtx_outAudio->codec, NULL) < 0)
        {

            qDebug()<<"编码器打开失败，退出程序";
            return false;
        }

        //将AVCodecContext的成员复制到AVCodecParameters结构体
        avcodec_parameters_from_context(pFormatCtx_Out->streams[1]->codecpar, pCodecCtx_outAudio);
    }

    if (!(pFormatCtx_Out->oformat->flags & AVFMT_NOFILE))
    {
        int ret,counter=0;
        do
        {
            ret=avio_open2(&pFormatCtx_Out->pb, outFileName, AVIO_FLAG_WRITE,NULL, NULL);
            QThread::msleep(100);
            ++counter;
        }while(ret<0&&counter<50);

        if( ret< 0)
        {
            qDebug()<<ret<<" can not open output file handle!\n";
            return false;
        }
    }


    if( avformat_write_header(pFormatCtx_Out, NULL)< 0)
    {
        qDebug()<<" can not write the header of the output file!\n";
        return false;
    }

    outFileOpened=true;

    videoConvertContext=sws_getContext(pCodecPra_inVideo->width,pCodecPra_inVideo->height,pCodecCtx_inVideo->pix_fmt,pCodecCtx_outVideo->width,pCodecCtx_outVideo->height,pCodecCtx_outVideo->pix_fmt,SWS_BICUBIC,nullptr,nullptr,nullptr);

    if (!videoConvertContext)
        return false;
    //申请视频缓存
    videoFrameSize=av_image_get_buffer_size(pCodecCtx_outVideo->pix_fmt, pCodecCtx_outVideo->width, pCodecCtx_outVideo->height,1);
    fifo_video=av_fifo_alloc(20 * videoFrameSize);

    audioConvertContext=swr_alloc_set_opts(NULL,pCodecCtx_outAudio->channel_layout,pCodecCtx_outAudio->sample_fmt,pCodecCtx_outAudio->sample_rate,pCodecCtx_inAudio->channel_layout, pCodecCtx_inAudio->sample_fmt,pCodecCtx_inAudio->sample_rate,0,NULL);
    if (!audioConvertContext || swr_init(audioConvertContext) < 0)
        return false;
    //申请音频缓存
    fifo_audio = av_audio_fifo_alloc(pCodecCtx_outAudio->sample_fmt,pCodecCtx_outAudio->channels,500*pCodecCtx_outAudio->frame_size);

    return true;
}

//重载显示函数
void sender_VideoAndAudio::sendPaintEvent()
{
    av_image_fill_arrays(localVideoFrame->data,localVideoFrame->linesize,localPictureBuffer,pCodecCtx_inVideo->pix_fmt,pCodecCtx_inVideo->width,pCodecCtx_inVideo->height,1);

    if(av_fifo_size(fifo_local)>=localBufferSize)
    {
        CapSection.lock();
        av_fifo_generic_read( fifo_local, localPictureBuffer,  localBufferSize, NULL);
        CapSection.unlock();
    }
    else
    {
        av_frame_unref(localVideoFrame);
        av_frame_unref(YUVframe);
        return;
    }

    YUVframe->width=pCodecCtx_inVideo->width;
    YUVframe->height=pCodecCtx_inVideo->height;
    YUVframe->format=AV_PIX_FMT_YUV420P;

    av_frame_get_buffer(YUVframe, 1);
    sws_scale( imgConvertContext,
               localVideoFrame->data,
               localVideoFrame->linesize,0,
               YUVframe->height,YUVframe->data,YUVframe->linesize);

    int bytes =0, uv=YUVframe->height/2;

    out_buffer=(uint8_t*)av_malloc(av_image_get_buffer_size(AV_PIX_FMT_YUV420P,YUVframe->width,YUVframe->height,1));
    av_image_fill_arrays(YUVframe->data,
                         YUVframe->linesize,
                         out_buffer,
                         AV_PIX_FMT_YUV420P,
                         YUVframe->width,
                         YUVframe->height,1);

    for(int i=0;i<YUVframe->height;++i){
        memcpy(out_buffer+bytes,
               YUVframe->data[0]+YUVframe->linesize[0]*i,
                YUVframe->width);
        bytes+=YUVframe->width;
    }

    for(int i=0;i<uv;++i){
        memcpy(out_buffer+bytes,
               YUVframe->data[1]+YUVframe->linesize[1]*i,
                YUVframe->width/2);
        bytes+=YUVframe->width/2;
    }

    for(int i=0;i<uv;++i){
        memcpy(out_buffer+bytes,
               YUVframe->data[2]+YUVframe->linesize[2]*i,
                YUVframe->width/2);
        bytes+=YUVframe->width/2;
    }
    qDebug()<<YUVframe->width<<" "<<YUVframe->height;

    emit sender_sendYUV(out_buffer, YUVframe->width, YUVframe->height);
    qDebug()<<out_buffer;
    av_frame_unref(localVideoFrame);
    av_frame_unref(YUVframe);
}

bool sender_VideoAndAudio::ScreenCapThreadProc()
{
    AVPacket *pkt=av_packet_alloc();
    AVFrame	*frame=av_frame_alloc();
    AVFrame *picture = av_frame_alloc();

    int height =pCodecCtx_outVideo->height;
    int width =pCodecCtx_outVideo->width;
    int y_size=height*width;
    int size = av_image_get_buffer_size(pCodecCtx_outVideo->pix_fmt, width, height,1);
    unsigned char* video_buf=(unsigned char*)av_malloc((size_t)5*size);

    while( bCap)
    {
        //Sleep(5);
        if (av_read_frame( pFormatCtx_inVideo, pkt) < 0)
        {
            av_packet_unref(pkt);
            av_frame_unref(picture);
            av_frame_unref(frame);
            qDebug()<<"empty packet";
            continue;
        }

        if(pkt->stream_index ==  videoIndex)
        {
            if (avcodec_send_packet( pCodecCtx_inVideo,pkt) < 0)
            {
                qDebug()<<"SEND Decode Error.（发送解码包错误）\n";
                av_packet_unref(pkt);
                av_frame_unref(picture);
                av_frame_unref(frame);
                continue;
            }

            if (avcodec_receive_frame( pCodecCtx_inVideo,frame) < 0)
            {
                qDebug()<<"RECEIVE Decode Error.（接受解码包错误）\n";
                av_packet_unref(pkt);
                av_frame_unref(picture);
                av_frame_unref(frame);
                continue;
            }

            av_image_fill_arrays(picture->data,picture->linesize, video_buf, pCodecCtx_outVideo->pix_fmt,width,height,1);
            if(sws_scale( videoConvertContext,frame->data, frame->linesize, 0,height, picture->data, picture->linesize)<0)
                qDebug()<<"FAILED video frame sws_scale";

            //qDebug()<<av_fifo_space( fifo_video) <<"  "<< size;
            if (av_fifo_space( fifo_video) >= size)
            {
                VideoSection.lock();
                if( codecIndex==1|| codecIndex==2)
                {
                    av_fifo_generic_write( fifo_video, picture->data[0],  y_size, NULL);//Y
                    av_fifo_generic_write( fifo_video, picture->data[1], y_size/2, NULL);//U、V
                }
                else
                {
                    av_fifo_generic_write( fifo_video, picture->data[0],  y_size, NULL);//Y
                    av_fifo_generic_write( fifo_video, picture->data[1], y_size/4, NULL);//U
                    av_fifo_generic_write( fifo_video, picture->data[2],y_size/4, NULL);//V
                }
                VideoSection.unlock();
            }

            if (av_fifo_space( fifo_local) >=  localBufferSize)
            {
                CapSection.lock();
                av_fifo_generic_write( fifo_local, frame->data[0],   localBufferSize, NULL);
                CapSection.unlock();
            }

        }
        av_frame_unref(frame);
        av_frame_unref(picture);
        av_packet_unref(pkt);
    }

    av_frame_free(&frame);
    av_frame_free(&picture);
    av_packet_free(&pkt);
    av_free(video_buf);
    qDebug()<<"video_done";
    return true;
}

bool sender_VideoAndAudio::AudioCapThreadProc()
{
    AVPacket *pkt=av_packet_alloc();
    AVFrame	*frame=av_frame_alloc();
    AVFrame *sample=av_frame_alloc();
    int dst_nb_samples, out_nb_samples ;
    // int size =av_samples_get_buffer_size(NULL, audioCodecCtx->channels, audioCodecCtx->frame_size, audioCodecCtx->sample_fmt,1);

    while( bCap)
    {
        //Sleep(5);
        if(av_read_frame( pFormatCtx_inAudio,pkt) < 0)
        {
            av_packet_unref(pkt);
            av_frame_unref(frame);
            av_frame_unref(sample);
            continue;
        }
        if(pkt->stream_index ==  audioIndex)
        {
            if (avcodec_send_packet( pCodecCtx_inAudio,pkt) < 0)
            {
                qDebug()<<"SEND Decode Error.（发送解码包错误）\n";
                av_packet_unref(pkt);
                av_frame_unref(frame);
                av_frame_unref(sample);
                continue;
            }
            if (avcodec_receive_frame( pCodecCtx_inAudio,frame) < 0)
            {
                qDebug()<<"RECEIVE Decode Error.（接受解码包错误）\n";
                av_packet_unref(pkt);
                av_frame_unref(frame);
                av_frame_unref(sample);
                continue;
            }

            dst_nb_samples = av_rescale_rnd( swr_get_delay( audioConvertContext, frame->sample_rate) +  frame->nb_samples,  pCodecCtx_outAudio->sample_rate, frame->sample_rate, AV_ROUND_UP);
            out_nb_samples = av_rescale_rnd(frame->nb_samples,  pCodecCtx_outAudio->sample_rate,  frame->sample_rate, AV_ROUND_UP);

            if(dst_nb_samples > out_nb_samples)
                out_nb_samples = dst_nb_samples;

            sample->nb_samples=out_nb_samples;

            int bufsize =av_samples_get_buffer_size(sample->linesize, pCodecCtx_outAudio->channels,sample->nb_samples, pCodecCtx_outAudio->sample_fmt,1);
            unsigned char* audio_buf=(unsigned char*)av_malloc((size_t)5*bufsize);
            av_samples_fill_arrays(sample->data,sample->linesize,audio_buf, pCodecCtx_outAudio->channels, sample->nb_samples,  pCodecCtx_outAudio->sample_fmt, 1);

            if(swr_convert( audioConvertContext, sample->data, sample->nb_samples, (const uint8_t**)frame->data, frame->nb_samples)<0)
                qDebug()<<"FAILED audio frame swr_convert";

            //qDebug()<<av_audio_fifo_space( fifo_audio) <<"  "<< bufsize;
            if (av_audio_fifo_space( fifo_audio) >= bufsize)
            {
                AudioSection.lock();
                av_audio_fifo_write( fifo_audio, (void **)sample->data,sample->nb_samples);
                AudioSection.unlock();
            }

            av_free(audio_buf);
        }
        av_packet_unref(pkt);
        av_frame_unref(frame);
        av_frame_unref(sample);
    }
    av_frame_free(&frame);
    av_frame_free(&sample);
    av_packet_free(&pkt);
    qDebug()<<"audio_done";
    return true;
}

bool sender_VideoAndAudio::EncodeThreadProc()
{
    AVFrame *picture = av_frame_alloc();
    AVPacket  *v_pkt=av_packet_alloc();
    AVFrame *sample=av_frame_alloc();
    AVPacket *a_pkt=av_packet_alloc();

    AVBSFContext *bsf_ctx;
    const AVBitStreamFilter *filter = av_bsf_get_by_name("hevc_mp4toannexb");
    if(!filter)
    {
        av_log(NULL,AV_LOG_ERROR,"Unkonw bitstream filter");
    }
    int ret = av_bsf_alloc(filter, &bsf_ctx);
    if(ret)
    {
        qDebug()<<"BSF INITIAL FAIL";
    }


    int size = av_image_get_buffer_size( pCodecCtx_outVideo->pix_fmt,  pCodecCtx_outVideo->width,  pCodecCtx_outVideo->height,1);
    unsigned char *video_buf=(unsigned char*)av_malloc((size_t)5*size);

    int sizeVideo=0;
    int sizeAudio=0;
    //const int factor=(videoCodecCtx->time_base.num*pFormatCtx_Out->streams[0]->time_base.den)/(videoCodecCtx->time_base.den * pFormatCtx_Out->streams[0]->time_base.num);
    uint64_t cur_pts_v=0, cur_pts_a=0, VideoFrameIndex = 0, AudioFrameIndex = 0;

    QTime mid=QTime::currentTime();
    int offset= start.msecsTo(mid)/1000;
    while(true)
    {
        QThread::msleep(1);
        sizeAudio = av_audio_fifo_size( fifo_audio);
        sizeVideo = av_fifo_size( fifo_video);

        //leave loop after the buffer is empty
        if ( fifo_audio &&  fifo_video)
        {
            //if (sizeVideo<=  videoCodecCtx->frame_size && ! bCap)
            if (sizeAudio<=  pCodecCtx_outAudio->frame_size && sizeVideo<=  pCodecCtx_outVideo->frame_size && ! bCap)
                break;
        }
        else
            break;

        if (sizeAudio<  pCodecCtx_outAudio->frame_size && ! bCap)
        {
            cur_pts_a = 0x7fffffffffffffff;
        }

        if(sizeAudio>=
                ( pCodecCtx_outAudio->frame_size > 0 ?  pCodecCtx_outAudio->frame_size : 1024))
        {
            sample->nb_samples = ( pCodecCtx_outAudio->frame_size>0 ?  pCodecCtx_outAudio->frame_size: 1024);
            sample->channel_layout =  pCodecCtx_outAudio->channel_layout;
            sample->format =  pCodecCtx_outAudio->sample_fmt;
            sample->sample_rate =  pCodecCtx_outAudio->sample_rate;
            av_frame_get_buffer(sample, 1);

            AudioSection.lock();
            av_audio_fifo_read( fifo_audio, (void **)sample->data, sample->nb_samples);
            AudioSection.unlock();

            sample->pts = AudioFrameIndex *  pCodecCtx_outAudio->frame_size;

            int error;
            error=avcodec_send_frame( pCodecCtx_outAudio, sample );
            if ( error< 0)
            {
                qDebug()<<error<<"SEND encodeAUDIO Error.\n";
            }

            error=avcodec_receive_packet( pCodecCtx_outAudio,a_pkt);
            if (error < 0)
            {
                qDebug()<<error<<"RECEIVE encodeAUDIO Error.\n";
                av_frame_unref(sample);
                av_packet_unref(a_pkt);
                continue;
            }

            a_pkt->stream_index = 1;

            //转换时间轴
            av_packet_rescale_ts(a_pkt, pCodecCtx_outAudio->time_base, pFormatCtx_Out->streams[1]->time_base);

            cur_pts_a = a_pkt->pts;

            /*
            av_interleaved_write_frame( pFormatCtx_Out, a_pkt);
            av_packet_unref(a_pkt);
           */
            WriteSection.lock();
            writeBuffer.enqueue(a_pkt);
            if(writeBuffer.size()>100)
                av_packet_unref(writeBuffer.dequeue());
            WriteSection.unlock();
            bufferEmpty.wakeAll();

            ++AudioFrameIndex;
            av_frame_unref(sample);
        }

        //compare timestamp
        if(av_compare_ts(cur_pts_v,  pFormatCtx_Out->streams[0]->time_base, cur_pts_a,  pFormatCtx_Out->streams[1]->time_base) > 0)
            continue;

        if (sizeVideo <  pCodecCtx_outVideo->frame_size && ! bCap)
        {
            cur_pts_v = 0x7fffffffffffffff;
        }
        if(sizeVideo >=  videoFrameSize)
        {
            // qDebug()<<picture->height;
            picture->height =  pCodecCtx_outVideo->height;
            picture-> width =  pCodecCtx_outVideo->width;
            picture->format= pCodecCtx_outVideo->pix_fmt;
            // qDebug()<<picture->height;
            av_image_fill_arrays(picture->data,picture->linesize,  video_buf, pCodecCtx_outVideo->pix_fmt,picture->width,picture->height,1);

            VideoSection.lock();
            av_fifo_generic_read( fifo_video, video_buf,  size, NULL);
            VideoSection.unlock();

            /*
             * videoCodecCtx->time_base.num==1
             * videoCodecCtx->time_base.den==20
             * fps==20
             * 由于受到录屏帧率以及编解码性能的限制，时间轴的设置有偏差
            */
            picture->pts =VideoFrameIndex+offset;
            //picture->pts =VideoFrameIndex;
            int error;
            error =avcodec_send_frame( pCodecCtx_outVideo, picture );
            if (error< 0)
            {
                qDebug()<<error<<"SEND encodeVIDEO Error.\n";
            }

            error=avcodec_receive_packet( pCodecCtx_outVideo,v_pkt) ;
            if (error< 0)
            {
                qDebug()<<error<<"RECEIVE encodeVIDEO Error.\n";
                av_frame_unref(picture);
                av_packet_unref(v_pkt);
                continue;
            }

            v_pkt->stream_index = 0;
            //转换时间轴
            av_packet_rescale_ts(v_pkt, pCodecCtx_outVideo->time_base, pFormatCtx_Out->streams[0]->time_base);
            cur_pts_v = v_pkt->pts;

            //把pkt数据推送到filter中去
            ret = av_bsf_send_packet(bsf_ctx, v_pkt);
            if(ret < 0)
            {
                qDebug()<<"BIT STREAM FILTER"<<ret;
                //推送失败，做异常处理
                av_frame_unref(picture);
                av_packet_unref(v_pkt);
                continue;
            }
            //获取处理后的数据，用同一个pkt
            ret = av_bsf_receive_packet(bsf_ctx, v_pkt);
            if(ret < 0)
            {
                qDebug()<<"BIT STREAM FILTER"<<ret;
                //读取失败，做异常处理
                av_frame_unref(picture);
                av_packet_unref(v_pkt);
                continue;
            }

            /*
            av_interleaved_write_frame( pFormatCtx_Out, v_pkt);
            av_packet_unref(v_pkt);
            */
            WriteSection.lock();
            writeBuffer.enqueue(v_pkt);
            if(writeBuffer.size()>100)
                av_packet_unref(writeBuffer.dequeue());
            WriteSection.unlock();
            bufferEmpty.wakeAll();

            ++VideoFrameIndex;
            av_frame_unref(picture);
        }
    }

    QTime end=QTime::currentTime();
    qDebug()<<"time(ms): "<< start.msecsTo(end);
    qDebug()<<"frame written:"<<VideoFrameIndex;

    bufferEmpty.wakeAll();
    av_bsf_free(&bsf_ctx);
    av_free(video_buf);
    av_packet_free(&a_pkt);
    av_frame_free(&sample);
    av_packet_free(&v_pkt);
    av_frame_free(&picture);
    qDebug()<<"ENCODE_done";
    return true;

}

bool sender_VideoAndAudio::WritePacketThreadProc()
{
    AVPacket  *pkt;

    while(! writeBuffer.empty() || bCap )
    {
        WriteSection.lock();

        if(writeBuffer.empty())
            bufferEmpty.wait(&WriteSection);
        if(writeBuffer.empty())
        {
            WriteSection.unlock();
            break;
        }

        pkt=writeBuffer.dequeue();
        WriteSection.unlock();

        av_interleaved_write_frame( pFormatCtx_Out, pkt);
        av_packet_unref(pkt);
    }
    qDebug()<<"WRITE_done";
    return true;
}
