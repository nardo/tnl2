# -------------------------------------------------
# Project created by QtCreator 2010-02-25T18:35:26
# -------------------------------------------------
QT += opengl
TARGET = tnl2_test
TEMPLATE = app
SOURCES += main.cpp \
    mainwindow.cpp \
    tnl2_test_widget.cpp \
    test_game.cpp
HEADERS += mainwindow.h \
    tnl2_test_widget.h \
    test_player.h \
    test_net_interface.h \
    test_game.h \
    test_connection.h \
    test_building.h
FORMS += mainwindow.ui
INCLUDEPATH += ../tnl2 \
    ../../torque_sockets/platform_library \
    ../../torque_sockets/lib/libtomcrypt/src/headers
LIBS += -L../../torque_sockets/lib/libtomcrypt -ltomcrypt
