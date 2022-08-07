#include "initialization_dialog.h"
#include "ui_initialization_dialog.h"
#include <QPushButton>

Initialization_Dialog::Initialization_Dialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::Initialization_Dialog)
{
    ui->setupUi(this);
    ui->m_deviceBox->clear();
    //ui->label->setText("音频输入设备选择");
    const QList<QAudioDevice> devices = m_devices->audioInputs();
    for (const QAudioDevice &deviceInfo: devices)
        ui->m_deviceBox->addItem(deviceInfo.description(), QVariant::fromValue(deviceInfo));
    connect(ui->buttonBox, &QDialogButtonBox::clicked, this, &Initialization_Dialog::on_buttonBox_clicked);
}

Initialization_Dialog::~Initialization_Dialog()
{
    delete ui;
}

void Initialization_Dialog::on_buttonBox_clicked(QAbstractButton *button)
{
    if( button==ui->buttonBox->button(QDialogButtonBox::Ok) )   //判断按下的是否为"确定”按钮
    {
        audioDevice=ui->m_deviceBox->currentText();
        //没有合规性检查
      QString  IPAddress=ui->IPMuticastAddress->toPlainText();
      QString  IPPort=ui->IPMuticastPort->toPlainText();
      outputAddress="rtp://"+IPAddress+":"+IPPort;//格式类似于“rtp://234.234.234.234:23334"
      wordMeetingPort=ui->WordmeetingPort->toPlainText();
      wordMeetingAddress=ui->IPMuticastAddress->toPlainText();
    }
    else if(button == ui->buttonBox->button(QDialogButtonBox::Cancel))
        close();//必须处理关闭情况
}
