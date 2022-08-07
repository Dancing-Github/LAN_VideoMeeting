QT       += core gui
QT       += openglwidgets
QT       += opengl
QT       += multimedia
QT       += concurrent
QT       += network


greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++20

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0
SOURCES += \
    decodethread.cpp \
    initialization_dialog.cpp \
    main.cpp \
    mainwindow.cpp \
    playaudio.cpp \
    playwidget.cpp \
    sender_videoandaudio.cpp \
    word_meeting.cpp

HEADERS += \
    decodethread.h \
    initialization_dialog.h \
    mainwindow.h \
    playaudio.h \
    playwidget.h \
    sender_videoandaudio.h \
    word_meeting.h

FORMS += \
    initialization_dialog.ui \
    mainwindow.ui

#添加库
INCLUDEPATH += $$PWD/ffmpeg/include

LIBS    += $$PWD/ffmpeg/lib/avcodec.lib \
            $$PWD/ffmpeg/lib/avdevice.lib \
            $$PWD/ffmpeg/lib/avfilter.lib \
            $$PWD/ffmpeg/lib/avformat.lib \
            $$PWD/ffmpeg/lib/avutil.lib \
            $$PWD/ffmpeg/lib/postproc.lib \
            $$PWD/ffmpeg/lib/swresample.lib \
            $$PWD/ffmpeg/lib/swscale.lib \


# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

DISTFILES +=
