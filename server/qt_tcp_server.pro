QT       = core network
QT      -= gui
CONFIG  += c++17 console
CONFIG  -= app_bundle
TARGET   = qt_tcp_server

SOURCES += \
    main.cpp \
    mytcpserver.cpp

HEADERS += \
    mytcpserver.h
