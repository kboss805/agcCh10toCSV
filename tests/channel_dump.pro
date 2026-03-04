QT += core gui
CONFIG += console c++17
CONFIG -= app_bundle

TARGET = channel_dump

INCLUDEPATH += ../include
INCLUDEPATH += ../lib/irig106/include

SOURCES += channel_dump.cpp \
           ../src/chapter10reader.cpp \
           ../src/channeldata.cpp \
           ../lib/irig106/src/irig106ch10.c \
           ../lib/irig106/src/i106_time.c \
           ../lib/irig106/src/i106_data_stream.c \
           ../lib/irig106/src/i106_decode_time.c \
           ../lib/irig106/src/i106_decode_tmats.c \
           ../lib/irig106/src/i106_decode_tmats_g.c \
           ../lib/irig106/src/i106_decode_tmats_r.c \
           ../lib/irig106/src/i106_decode_tmats_m.c \
           ../lib/irig106/src/i106_decode_tmats_p.c \
           ../lib/irig106/src/i106_decode_tmats_b.c \
           ../lib/irig106/src/i106_decode_tmats_c.c \
           ../lib/irig106/src/i106_decode_tmats_d.c

HEADERS += ../include/chapter10reader.h \
           ../include/channeldata.h \
           ../include/constants.h

LIBS += -lws2_32
