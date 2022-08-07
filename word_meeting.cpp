#include "word_meeting.h"
#include<QHostInfo>
#include<QVariant>
word_meeting::word_meeting(QObject *parent)
    : QObject{parent}
{
    udpsocket=new QUdpSocket;
    udpsocket->setSocketOption(QAbstractSocket::MulticastTtlOption,1);
    connect(udpsocket,&QUdpSocket::readyRead,this,&word_meeting::readMsg);
    //udpsocket->setSocketOption(QAbstractSocket::ReceiveBufferSizeSocketOption,1024*1024*8);

}

QString word_meeting::getip()
{
    QString hostName = QHostInfo::localHostName();//本机主机名
    QHostInfo hostInfo = QHostInfo::fromName(hostName);
    QString localIP = " ";
    QList<QHostAddress> addList = hostInfo.addresses();
    if(!addList.isEmpty())
    {
        for(int i=0;i<addList.count();++i)
        {
            QHostAddress aHost = addList.at(i);
            if(QAbstractSocket::IPv4Protocol == aHost.protocol())
            {
                localIP = aHost.toString();
                break;
            }
        }
    }
    return localIP;
}

bool word_meeting::bind_des(QString ip,QString port)
{
    groupaddress=QHostAddress(ip);
    groupport=port.toInt();
    if(udpsocket->bind(QHostAddress::AnyIPv4,groupport,QUdpSocket::ShareAddress| QUdpSocket::ReuseAddressHint))
    {
        if(udpsocket->joinMulticastGroup(groupaddress))
        {

            return 1;
        }
        return 0;
    }
    return 0;
}

bool word_meeting::disbind_des()
{
    if(udpsocket->leaveMulticastGroup(groupaddress))
    {
        udpsocket->abort();
        return 1;
    }
    return 0;

}

bool word_meeting::sendMsg(QString msg)
{
    QByteArray msg_array=msg.toUtf8();
    if(udpsocket->writeDatagram(msg_array,groupaddress,groupport))
    {
        return 1;
    }
    return 0;
}

void word_meeting::readMsg()
{
    QByteArray datagram;
    datagram.resize(udpsocket->pendingDatagramSize());
    QHostAddress peeraddr;
    quint16 peerport;
    udpsocket->readDatagram(datagram.data(),datagram.size(),&peeraddr,&peerport);
    QString msg=datagram.data();
    datagram.clear();
    QString tot_msg="[from "+peeraddr.toString()+":"+QString::number(peerport)+" ] "+msg;
    emit getMsg(tot_msg);

}
