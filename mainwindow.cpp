#include "mainwindow.h"
#include "ui_mainwindow.h"
#include<QPainter>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    /*decodethread=nullptr;
        videothread=nullptr;
        audioplay=nullptr;*/
    qDebug()<<"initializing MainWindow 0";
    decodethread=new DecodeThread(this);
    videothread=new VideoThread(this);
    audioplay=new AudioPlay(this);
    qDebug()<<"initializing MainWindow 1";
    QObject::connect(videothread,SIGNAL(updateYUV(uchar*,uint,uint)),ui->playwidget,SLOT(slotShowYuv(uchar*,uint,uint)));
    QObject::connect(decodethread,SIGNAL(sigGetSampleRate(int,bool)),audioplay,SLOT(init(int,bool)));
    QObject::connect(decodethread,SIGNAL(happenFail()),this,SLOT(errorStop()));
    qDebug()<<"initializing MainWindow 2";
    this->m_p_getS=true;
    this->m_p_shareS=true;
    connect(ui->p_getscreen,&QPushButton::clicked,[&]{
        on_btnInfoNotice_clicked();
        if(this->m_p_shareS)
        {
            if(this->ui->p_getscreen->text()=="接收共享")
            {
                if(this->m_p_getS)
                {
                    this->m_p_getS=false;
                    this->getShareS();
                    this->ui->p_getscreen->setText("停止接收");
                }
            }
            else
            {
                if(QMessageBox::Yes == QMessageBox::question(this,"提示","是否要停止接收？",QMessageBox::Yes | QMessageBox::No))
                {
                    this->ui->p_getscreen->setText("接收共享");
                    this->stopShareS();
                    videothread->init();
                    decodethread->init();
                    this->m_p_getS=true;
                }

            }
        }
        else
        {
            QMessageBox::question(this,"提示","正在进行屏幕共享！",QMessageBox::Ok);
        }

    });

    qDebug()<<"initializing MainWindow 3";
    //-------------------------------------------------

    sender=nullptr;
    wordMeeting=nullptr;

    connect(ui->p_sharescreen,&QPushButton::clicked,[&]{
        on_btnInfoNotice_clicked();
        if(this->m_p_getS)
        {
            if(this->ui->p_sharescreen->text()=="共享屏幕")
            {
                this->m_p_shareS=false;
                this->sender_begin();
                qDebug()<<"999999999999999";
                decodethread->setType();
                this->getShareS();
                this->ui->p_sharescreen->setText("停止共享");
            }
            else
            {
                this->ui->p_sharescreen->setText("共享屏幕");
                this->sender_stop();

                this->stopShareS();
                videothread->init();
                decodethread->init();
                qDebug()<<"\\\\\\\\\\\\\\\\\\\\";
                this->m_p_shareS=true;
            }
        }
        else
        {
            QMessageBox::question(this,"提示","正在接收屏幕共享！",QMessageBox::Ok);
        }
    });
    qDebug()<<"initializing MainWindow 4";
    connect(ui->p_inputRTP,&QPushButton::clicked,[&]{
        on_btnInfoNotice_clicked();
        if(this->m_p_getS && this->m_p_shareS)
        {
            Initialization_Dialog d;
            d.show();
            d.exec();
            if(!d.outputAddress.isEmpty() )
            {
                audioDevice=d.audioDevice;
                outputAddress=d.outputAddress;
                address=d.wordMeetingAddress;
                port=d.wordMeetingPort;

                if(wordMeeting)
                {
                    wordMeeting->disconnect(SIGNAL(getMsg(QString)));
                    //ui->p_sendWORD->disconnect(SIGNAL(QPushButton::clicked()));
                    delete wordMeeting;
                }

                wordMeeting=new word_meeting(this);
                //void(word_meeting::* GETMSG)(QString)=&word_meeting::getMsg;
                QObject::connect(wordMeeting,SIGNAL(getMsg(QString)),this,SLOT(show_readText(QString)));
                connect(ui->p_sendWORD,&QPushButton::clicked,this,&MainWindow::send_writeText,Qt::UniqueConnection);

                if(wordMeeting->bind_des(address,port))
                {
                    ui->textBrowser->append("**加入组播成功");
                }
                else
                {
                    ui->textBrowser->append("**绑定端口失败");
                }

                decodethread->setFilePath(outputAddress);
            }


            qDebug()<<audioDevice;
            qDebug()<<outputAddress;
        }
        else
        {
            QMessageBox::question(this,"提示","正在使用该组播号，请先结束屏幕共享或接收共享！",QMessageBox::Ok);
        }


    });



    qDebug()<<"initializing MainWindow 5";
}

MainWindow::~MainWindow()
{
    delete ui;
    sender_stop();
    stopShareS();
}

void MainWindow::on_btnInfoNotice_clicked()
{
    ui->p_getscreen->setEnabled(false);
    ui->p_inputRTP->setEnabled(false);
    ui->p_sharescreen->setEnabled(false);
    QTimer::singleShot(1000, this, [=]() {
        ui->p_getscreen->setEnabled(true);
        ui->p_inputRTP->setEnabled(true);
        ui->p_sharescreen->setEnabled(true);
    });
}

PlayWidget* MainWindow::getPlayWidget()
{
    return ui->playwidget;
}

void MainWindow::getShareS()
{
    qDebug()<<"8888888888888888";
    decodethread->start();
    videothread->start();
}

void MainWindow::stopShareS()
{
    if(videothread)
    {

        //videothread->disconnect(SIGNAL(updateYUV(uchar*,uint,uint)));
        videothread->stop();
        QThread::msleep(500);
        DecodeThread::condition.wakeAll();
        DecodeThread::exist.wakeAll();
        videothread->requestInterruption();
        qDebug()<<"videothread111111111111";
        videothread->quit();
        videothread->wait();

        //delete videothread;
        //videothread=nullptr;
    }
    if(audioplay)
    {
        audioplay->stop();
        //QThread::msleep(500);
        qDebug()<<"audioplay111111111111";
        //delete audioplay;
        //audioplay=nullptr;
        qDebug()<<"985415321684";
    }
    if(decodethread)
    {
        //decodethread->disconnect(SIGNAL(sigGetSampleRate(int)));
        decodethread->stop();
        DecodeThread::enque.wakeAll();
        DecodeThread::audio_noEmpty.wakeAll();
        QThread::msleep(500);
        //decodethread->terminate();
        qDebug()<<"decodethread111111111111";
        decodethread->requestInterruption();
        decodethread->quit();
        decodethread->wait();
        decodethread->del();
        qDebug()<<"123549/765";
        if(!DecodeThread::audio_buffer)
        {
            av_free(DecodeThread::audio_buffer);
        }
        while(!DecodeThread::audio_outBuffer.empty())
        {
            av_free(DecodeThread::audio_outBuffer.dequeue().buffer);
        }
        if(!DecodeThread::out_buffer)
        {
            av_free(DecodeThread::out_buffer);
        }
        while(!DecodeThread::video_outBuffer.empty())
        {
            av_free(DecodeThread::video_outBuffer.dequeue().buffer);
        }
        for(auto itr=DecodeThread::video_outdata.begin();itr!=DecodeThread::video_outdata.end();++itr)
        {
            if(itr->buffer)
            {
                av_free(itr->buffer);
                itr->buffer=nullptr;
            }

        }
        for(auto itr=DecodeThread::audio_outdata.begin();itr!=DecodeThread::audio_outdata.end();++itr)
        {
            if(itr->buffer)
            {
                av_free(itr->buffer);
                itr->buffer=nullptr;
            }
        }
        if(!DecodeThread::ptr)
        {
            av_free(DecodeThread::ptr);
        }
        //delete decodethread;
        //decodethread=nullptr;


    }

    qDebug()<<"sjkdfhbdjkshuv";
}

void MainWindow::errorStop()
{
    if(videothread)
    {
        //videothread->disconnect(SIGNAL(updateYUV(uchar*,uint,uint)));
        videothread->stop();
        DecodeThread::condition.wakeAll();
        DecodeThread::exist.wakeAll();
        videothread->requestInterruption();
        videothread->quit();
        videothread->wait();
        //delete videothread;
        //videothread=nullptr;
    }
    if(audioplay)
    {
        audioplay->stop();
        //delete audioplay;
        //audioplay=nullptr;
    }
    if(decodethread)
    {
        //decodethread->disconnect(SIGNAL(sigGetSampleRate(int)));
        decodethread->stop();
        DecodeThread::enque.wakeAll();
        DecodeThread::audio_noEmpty.wakeAll();
        //decodethread->terminate();
        decodethread->requestInterruption();
        decodethread->quit();
        decodethread->wait();
        decodethread->del();
        if(!DecodeThread::audio_buffer)
        {
            av_free(DecodeThread::audio_buffer);
        }
        while(!DecodeThread::audio_outBuffer.empty())
        {
            av_free(DecodeThread::audio_outBuffer.dequeue().buffer);
        }
        if(!DecodeThread::out_buffer)
        {
            av_free(DecodeThread::out_buffer);
        }
        while(!DecodeThread::video_outBuffer.empty())
        {
            av_free(DecodeThread::video_outBuffer.dequeue().buffer);
        }
        for(auto itr=DecodeThread::video_outdata.begin();itr!=DecodeThread::video_outdata.end();++itr)
        {
            if(itr->buffer)
            {
                av_free(itr->buffer);
                itr->buffer=nullptr;
            }

        }
        for(auto itr=DecodeThread::audio_outdata.begin();itr!=DecodeThread::audio_outdata.end();++itr)
        {
            if(itr->buffer)
            {
                av_free(itr->buffer);
                itr->buffer=nullptr;
            }
        }
        if(!DecodeThread::ptr)
        {
            av_free(DecodeThread::ptr);
        }
        //delete decodethread;
        //decodethread=nullptr;
    }
    videothread->init();
    decodethread->init();
}

void MainWindow::sender_begin()
{
    if(sender)
        sender_stop();
    sender=new sender_VideoAndAudio(audioDevice,outputAddress,this);
    //connect(sender,&sender_VideoAndAudio:: error, this, &MainWindow::sender_error);

    if(!sender->sender_Initialize())
        sender_error("屏幕共享启动失败");

    //connect(sender,SIGNAL(sender_sendYUV(uchar*,uint,uint)),ui->playwidget,SLOT(slotShowYuv(uchar*,uint,uint)));
}

void MainWindow::sender_stop()
{
    if(sender)
    {
        //QObject::disconnect(sender,SIGNAL(sender_sendYUV(uchar*,uint,uint)),ui->playwidget,SLOT(slotShowYuv(uchar*,uint,uint)));
        delete sender;
    }
    sender =nullptr;
}

void MainWindow::sender_error(QString errMsg)
{
    QMessageBox::critical(this , "警告 " ,errMsg+"\n 请停止后再试一次" , QMessageBox::Ok | QMessageBox::Default | QMessageBox::Escape  ,NULL, 	NULL );
}

void MainWindow::show_readText(QString msg)
{
    //qDebug()<<"222222222222222222";
    ui->textBrowser->append(msg);
    QApplication::processEvents();
}

void MainWindow::send_writeText()
{
    //qDebug()<<"111111111111111";
    QString words=ui->textEdit->document()->toPlainText();
    //qDebug()<<"words: "<<words;
    wordMeeting->sendMsg(words);
    //ui->textBrowser->append("[multicast]"+words);
    ui->textEdit->clear();
}

