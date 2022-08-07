#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QDialog>
#include <QMessageBox>
#include "playwidget.h"
#include "decodethread.h"
#include "playaudio.h"
#include "sender_videoandaudio.h"
#include "initialization_dialog.h"
#include"word_meeting.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    PlayWidget* getPlayWidget();
private:
    void on_btnInfoNotice_clicked();

    Ui::MainWindow *ui;
    DecodeThread* decodethread;
    AudioPlay* audioplay;
    VideoThread* videothread;
    sender_VideoAndAudio* sender;
    word_meeting* wordMeeting;
    QString audioDevice;
    QString outputAddress;//格式类似于“rtp://234.234.234.234:23334"
    QString address, port;
    bool m_p_getS;
    bool m_p_shareS;
    bool m_stoping;
private slots:
    void getShareS();
    void stopShareS();
    void errorStop();
    void sender_begin();
    void sender_stop();
    void sender_error(QString errMsg);
    void show_readText(QString msg);
    void send_writeText();
};

#endif // MAINWINDOW_H
