QT += core gui widgets testlib

CONFIG += c++17 console
CONFIG -= app_bundle

TARGET = agcCh10toCSV_tests

INCLUDEPATH += \
    $$PWD/../include \
    $$PWD/../lib/irig106/include

win32 {
    LIBS += -lws2_32
}

# Application sources (exclude main.cpp to avoid duplicate main)
SOURCES += \
    $$PWD/../src/channeldata.cpp \
    $$PWD/../src/chapter10reader.cpp \
    $$PWD/../src/framesetup.cpp \
    $$PWD/../src/mainviewmodel.cpp \
    $$PWD/../src/mainview.cpp \
    $$PWD/../src/settingsdialog.cpp \
    $$PWD/../src/frameprocessor.cpp \
    $$PWD/../src/settingsmanager.cpp

# Application headers
HEADERS += \
    $$PWD/../include/channeldata.h \
    $$PWD/../include/chapter10reader.h \
    $$PWD/../include/constants.h \
    $$PWD/../include/framesetup.h \
    $$PWD/../include/mainviewmodel.h \
    $$PWD/../include/mainview.h \
    $$PWD/../include/frameprocessor.h \
    $$PWD/../include/settingsdata.h \
    $$PWD/../include/settingsdialog.h \
    $$PWD/../include/settingsmanager.h

# irig106 library sources
SOURCES += \
    $$PWD/../lib/irig106/src/irig106ch10.c \
    $$PWD/../lib/irig106/src/i106_time.c \
    $$PWD/../lib/irig106/src/i106_data_stream.c \
    $$PWD/../lib/irig106/src/i106_decode_time.c \
    $$PWD/../lib/irig106/src/i106_decode_tmats.c \
    $$PWD/../lib/irig106/src/i106_decode_tmats_g.c \
    $$PWD/../lib/irig106/src/i106_decode_tmats_r.c \
    $$PWD/../lib/irig106/src/i106_decode_tmats_m.c \
    $$PWD/../lib/irig106/src/i106_decode_tmats_p.c \
    $$PWD/../lib/irig106/src/i106_decode_tmats_b.c \
    $$PWD/../lib/irig106/src/i106_decode_tmats_c.c \
    $$PWD/../lib/irig106/src/i106_decode_tmats_d.c \
    $$PWD/../lib/irig106/src/i106_decode_pcmf1.c

# irig106 library headers
HEADERS += \
    $$PWD/../lib/irig106/include/irig106ch10.h \
    $$PWD/../lib/irig106/include/i106_data_stream.h \
    $$PWD/../lib/irig106/include/i106_decode_time.h \
    $$PWD/../lib/irig106/include/i106_time.h \
    $$PWD/../lib/irig106/include/i106_stdint.h \
    $$PWD/../lib/irig106/include/config.h \
    $$PWD/../lib/irig106/include/i106_decode_tmats.h \
    $$PWD/../lib/irig106/include/i106_decode_tmats_g.h \
    $$PWD/../lib/irig106/include/i106_decode_tmats_r.h \
    $$PWD/../lib/irig106/include/i106_decode_tmats_m.h \
    $$PWD/../lib/irig106/include/i106_decode_tmats_p.h \
    $$PWD/../lib/irig106/include/i106_decode_tmats_b.h \
    $$PWD/../lib/irig106/include/i106_decode_tmats_c.h \
    $$PWD/../lib/irig106/include/i106_decode_tmats_d.h \
    $$PWD/../lib/irig106/include/i106_decode_tmats_common.h \
    $$PWD/../lib/irig106/include/i106_decode_pcmf1.h

# Test sources
SOURCES += \
    main.cpp \
    tst_channeldata.cpp \
    tst_constants.cpp \
    tst_mainviewmodel_helpers.cpp \
    tst_mainviewmodel_state.cpp \
    tst_framesetup.cpp

# Test headers (needed for MOC processing)
HEADERS += \
    tst_channeldata.h \
    tst_constants.h \
    tst_mainviewmodel_helpers.h \
    tst_mainviewmodel_state.h \
    tst_framesetup.h
