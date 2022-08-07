#include "getstreamanddisplay.h"
#include "ui_getstreamanddisplay.h"

getStreamAndDisplay::getStreamAndDisplay(QOpenGLWidget *parent) :
    QOpenGLWidget(parent),
    ui(new Ui::getStreamAndDisplay)
{
    ui->setupUi(this);


    QTimer* timer=new QTimer(this);
    //connect(timer,&QTimer::timeout,this,&QWidget::repaint);
    connect(timer,&QTimer::timeout,this,static_cast<void (QWidget::*)()>(&QWidget::repaint));
    //可能repaint有重载
    timer->setInterval(20);
    timer->start();
    avformat_network_init();


    infilename="rtp://233.233.233.233:23333";
    outfilename="testReceive.mp4";

    ofmt=NULL;
    ifmt_ctx=NULL;
    ofmt_ctx=NULL;
    init();
    wirteHead();

}

getStreamAndDisplay::~getStreamAndDisplay()
{
    delete ui;
    free();
}

void getStreamAndDisplay::init()
{
    int ret=0;
    AVDictionary *d = NULL;
    av_dict_set(&d, "protocol_whitelist", "file,udp,rtp", 0);
    av_dict_set(&d,"probesize","51200000",0);
    av_dict_set(&d,"analyzeduration","1000000",0);
    //av_dict_set(&d,"max_analyse_duration","10",0);
    if(( ret=avformat_open_input(&ifmt_ctx,infilename,0,&d))<0)
    {
        qDebug()<<"不能打开输入上下文";
        return;
    }

    if((ret=avformat_find_stream_info(ifmt_ctx,0))<0)
    {
        qDebug()<<"不能查找流信息";
        return;
    }
    for(int i=0; i<ifmt_ctx->nb_streams; i++)
    {
        if(ifmt_ctx->streams[i]->codecpar->codec_type==AVMEDIA_TYPE_VIDEO)
        {
            videoindex=i;
            break;
        }
    }

    if(videoindex==-1)
    {
        qDebug()<<"can not find video stream";
        return;
    }

/*--------------------------------------------*/


    codecPar=ifmt_ctx->streams[videoindex]->codecpar;
    codecCtx=avcodec_alloc_context3(NULL);
    avcodec_parameters_to_context(codecCtx,codecPar);
    const AVCodec* codec=avcodec_find_decoder(codecCtx->codec_id);
    ret=avcodec_open2(codecCtx,codec,NULL);
    if(ret<0)
    {
        qDebug()<<"不能打开解码器";
        return;
    }
    pkt=av_packet_alloc();
    frame=av_frame_alloc();
    RGBframe=av_frame_alloc();
    imgConvertContext=sws_getContext(codecCtx->width,codecCtx->height,codecCtx->pix_fmt,
                                     codecCtx->width,codecCtx->height,
                                    AV_PIX_FMT_RGB32,SWS_BICUBIC,nullptr,nullptr,nullptr);
    outbuffer=(unsigned char*)av_malloc((size_t)av_image_get_buffer_size(AV_PIX_FMT_RGB32,codecCtx->width,codecCtx->height,1));
    av_image_fill_arrays(RGBframe->data,RGBframe->linesize,outbuffer,AV_PIX_FMT_RGB32,codecCtx->width,codecCtx->height,1);




/*----------------------------------------------------------*/



    avformat_alloc_output_context2(&ofmt_ctx,NULL,NULL,outfilename);

    if(!ofmt_ctx)
    {
        qDebug()<<"打不开输出上下文";
        return;
    }

    ofmt=ofmt_ctx->oformat;
    for(int i=0;i<ifmt_ctx->nb_streams;++i)
    {

        AVStream* instream=ifmt_ctx->streams[i];
        const AVCodec* inStr=avcodec_find_decoder(instream->codecpar->codec_id);
        const AVCodec* outStr =avcodec_find_decoder(instream->codecpar->codec_id);
        AVStream* outstream=avformat_new_stream(ofmt_ctx,outStr);
        if(!outstream)
        {
            qDebug()<<"打不开输出流";
            return;
        }

        AVCodecContext* CodecCtx=avcodec_alloc_context3(inStr);
        ret=avcodec_parameters_to_context(CodecCtx,instream->codecpar);
        if(ret<0)
        {
            qDebug()<<"parameter到context转换失败";
            return;
        }
        CodecCtx->codec_tag=0;
        if (ofmt_ctx->oformat->flags & AVFMT_GLOBALHEADER)
        {
            CodecCtx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
        }

        ret= avcodec_parameters_from_context(outstream->codecpar,CodecCtx);
        if(ret<0)
        {
            qDebug()<<"context到parameter转换失败";
            return;
        }


    }
    if (!(ofmt->flags & AVFMT_NOFILE))
    {
        ret = avio_open(&ofmt_ctx->pb, outfilename, AVIO_FLAG_WRITE);
        if (ret < 0) {
            qDebug()<<"打不开输出流";
            return;
        }
    }


    /*--------------------------------------------------------------*/



}

void getStreamAndDisplay::wirteHead()
{
    int ret=avformat_write_header(ofmt_ctx,NULL);
    if(ret<0)
    {
        qDebug()<<"不能写头";
        return;
    }
}



void getStreamAndDisplay::read()
{
    AVStream* instream;
    AVStream* outstream;


    while(1)
    {
        int ret=av_read_frame(ifmt_ctx,pkt);

        if(ret<0)
        {
            //break;
            qDebug()<<"得不到帧";
            break;
        }

        instream=ifmt_ctx->streams[pkt->stream_index];
        outstream=ofmt_ctx->streams[pkt->stream_index];
        if(frameindex==0)
        {
            firstPts=av_rescale_q_rnd(pkt->pts, instream->time_base, outstream->time_base, (AVRounding)(AV_ROUND_NEAR_INF|AV_ROUND_PASS_MINMAX));
            firstDts=av_rescale_q_rnd(pkt->dts, instream->time_base, outstream->time_base, (AVRounding)(AV_ROUND_NEAR_INF|AV_ROUND_PASS_MINMAX));
        }
        pkt->pts = av_rescale_q_rnd(pkt->pts, instream->time_base, outstream->time_base, (AVRounding)(AV_ROUND_NEAR_INF|AV_ROUND_PASS_MINMAX))-firstPts;
        pkt->dts = av_rescale_q_rnd(pkt->dts, instream->time_base, outstream->time_base, (AVRounding)(AV_ROUND_NEAR_INF|AV_ROUND_PASS_MINMAX))-firstDts;
        pkt->duration = av_rescale_q(pkt->duration, instream->time_base, outstream->time_base);
        pkt->pos = -1;

        if(pkt->stream_index==videoindex)
        {
            display(pkt);
            qDebug()<<"receive"<<frameindex<<" frames";
            ++frameindex;
            //av_usleep(20);
        }

        ret= av_interleaved_write_frame(ofmt_ctx,pkt);

        if(ret<0)
        {
            char* buff=new char[1024];
            qDebug()<<av_strerror(ret,buff,1024);
            delete[]buff;

        }
        QApplication::processEvents();//不加这句话，qt界面反应不过来去画画
        av_usleep(40);
        av_packet_unref(pkt);
    }
    endRead();

}


void getStreamAndDisplay::display(AVPacket *pkt)
{
    frame->height=codecCtx->height;
    frame->width=codecCtx->width;
    int ret=avcodec_send_packet(codecCtx,pkt);
    if(ret<0)
    {
        qDebug()<<"发送包解码错误";
        return;
    }
    ret=avcodec_receive_frame(codecCtx,frame);
    if(ret<0)
    {
        qDebug()<<"接收解码帧错误";
        return;
    }
    sws_scale(imgConvertContext,(const unsigned char* const*)frame->data,frame->linesize,0,codecCtx->height,RGBframe->data,RGBframe->linesize);
    QImage img((uchar*)RGBframe->data[0],codecCtx->width,codecCtx->height,QImage::Format_RGB32);
    QPainter painter(ui->openGLWidget);
    painter.setRenderHints(QPainter::Antialiasing, true);//抗锯齿
    painter.drawImage(ui->openGLWidget->pos(),img.scaled(ui->openGLWidget->width(),ui->openGLWidget->height()));
    painter.end();


}



void getStreamAndDisplay::endRead()
{
    int ret=av_write_trailer(ofmt_ctx);
    if(ret<0)
    {
        qDebug()<<"不能写文件尾";
        return;
    }
}

void getStreamAndDisplay::free()
{
    avformat_close_input(&ifmt_ctx);
    avio_close(ofmt_ctx->pb);
    avformat_free_context(ofmt_ctx);
    av_frame_free(&frame);
    av_frame_free(&RGBframe);
    sws_freeContext(imgConvertContext);

}
