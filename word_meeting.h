#ifndef WORD_MEETING_H
#define WORD_MEETING_H

#include <QObject>
#include<QUdpSocket>
class word_meeting : public QObject
{
    Q_OBJECT
public:
    explicit word_meeting(QObject *parent = nullptr);
    QString getip();
    bool bind_des(QString ip,QString port);
    bool disbind_des();
    bool sendMsg(QString msg);
    void readMsg();
private:
    QUdpSocket* udpsocket;
    QHostAddress groupaddress;
    quint16 groupport;
signals:
    void getMsg(QString msg);

};

#endif // WORD_MEETING_H
