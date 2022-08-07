#ifndef INITIALIZATION_DIALOG_H
#define INITIALIZATION_DIALOG_H

#include <QDialog>
#include <QString>
#include <QAudioSource>
#include <QMediaDevices>
#include <QAbstractButton>

namespace Ui {
class Initialization_Dialog;
}

class Initialization_Dialog : public QDialog
{
    Q_OBJECT

public:
    explicit Initialization_Dialog(QWidget *parent = nullptr);
    ~Initialization_Dialog();

    QString audioDevice;
    QString outputAddress;//格式类似于“rtp://234.234.234.234:23334"
    QString wordMeetingPort;
    QString wordMeetingAddress;

private slots:
    void on_buttonBox_clicked(QAbstractButton *button);
private:
    Ui::Initialization_Dialog *ui;
    QMediaDevices *m_devices = nullptr;
};

#endif // INITIALIZATION_DIALOG_H
