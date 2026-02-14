QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17

INCLUDEPATH += \
    $$PWD/include/ \
    $$PWD/lib/irig106/include/

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

win32 {
    LIBS += -lws2_32 # Need this for Windows 32-bit functions, specifically WSASocketW()
}

SOURCES += \
    src/channeldata.cpp \
    src/chapter10reader.cpp \
    src/framesetup.cpp \
    src/main.cpp \
    src/mainviewmodel.cpp \
    src/mainview.cpp \
    src/receivergridwidget.cpp \
    src/settingsdialog.cpp \
    src/timeextractionwidget.cpp \
    src/frameprocessor.cpp \
    src/settingsmanager.cpp \
    lib/irig106/src/irig106ch10.c \
    lib/irig106/src/i106_time.c \
    lib/irig106/src/i106_data_stream.c \
    lib/irig106/src/i106_decode_time.c \
    lib/irig106/src/i106_decode_tmats.c \
    lib/irig106/src/i106_decode_tmats_g.c \
    lib/irig106/src/i106_decode_tmats_r.c \
    lib/irig106/src/i106_decode_tmats_m.c \
    lib/irig106/src/i106_decode_tmats_p.c \
    lib/irig106/src/i106_decode_tmats_b.c \
    lib/irig106/src/i106_decode_tmats_c.c \
    lib/irig106/src/i106_decode_tmats_d.c \
    lib/irig106/src/i106_decode_pcmf1.c

HEADERS += \
    include/channeldata.h \
    include/chapter10reader.h \
    include/constants.h \
    include/framesetup.h \
    include/mainviewmodel.h \
    include/mainview.h \
    include/receivergridwidget.h \
    include/frameprocessor.h \
    include/settingsdata.h \
    include/settingsdialog.h \
    include/timeextractionwidget.h \
    include/settingsmanager.h \
    lib/irig106/include/irig106ch10.h \
    lib/irig106/include/i106_data_stream.h \
    lib/irig106/include/i106_decode_time.h \
    lib/irig106/include/i106_time.h \
    lib/irig106/include/i106_stdint.h \
    lib/irig106/include/config.h \
    lib/irig106/include/i106_decode_tmats.h \
    lib/irig106/include/i106_decode_tmats_g.h \
    lib/irig106/include/i106_decode_tmats_r.h \
    lib/irig106/include/i106_decode_tmats_m.h \
    lib/irig106/include/i106_decode_tmats_p.h \
    lib/irig106/include/i106_decode_tmats_b.h \
    lib/irig106/include/i106_decode_tmats_c.h \
    lib/irig106/include/i106_decode_tmats_d.h \
    lib/irig106/include/i106_decode_tmats_common.h \
    lib/irig106/include/i106_decode_pcmf1.h

RESOURCES += \
    resources/win11-dark.qss \
    resources/win11-light.qss \
    resources/icon.ico \
    resources/checkmark.svg \
    resources/chevron-down-dark.svg \
    resources/chevron-down-light.svg \
    resources/chevron-down-disabled-dark.svg \
    resources/chevron-down-disabled-light.svg \
    resources/folder-open.svg \
    resources/play.svg \
    resources/stop.svg

RC_FILE = agcCh10toCSV_resource.rc

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
