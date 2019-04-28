QT       += core

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++11 console
CONFIG -= app_bundle

# The following define makes your compiler emit warnings if you use
# any Qt feature that has been marked deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0



#######################linux##################################
unix{

}





#######################window##################################
win32{

INCLUDEPATH += -I ./include/ffmpeg

LIBS += $$PWD/lib/ffmpeg/win/libavcodec.dll.a \
        $$PWD/lib/ffmpeg/win/libavdevice.dll.a \
        $$PWD/lib/ffmpeg/win/libavfilter.dll.a \
        $$PWD/lib/ffmpeg/win/libavformat.dll.a \
        $$PWD/lib/ffmpeg/win/libavutil.dll.a \
        $$PWD/lib/ffmpeg/win/libpostproc.dll.a \
        $$PWD/lib/ffmpeg/win/libswresample.dll.a \
        $$PWD/lib/ffmpeg/win/libswscale.dll.a -lws2_32
}
################################################################s


SOURCES += \
        main.cpp

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
