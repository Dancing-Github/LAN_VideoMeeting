# pragma execution_character_set("utf-8")
#include <QApplication>
#include "mainwindow.h"


int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
//    QSurfaceFormat format;
//    format.setVersion(4,3);
//    format.setProfile(QSurfaceFormat::CoreProfile);
//    QSurfaceFormat::setDefaultFormat(format);

    MainWindow mainwindow;
    qDebug()<<"initializing  after";
    mainwindow.show();
    qDebug()<<"initializing  done";
    return a.exec();
}
