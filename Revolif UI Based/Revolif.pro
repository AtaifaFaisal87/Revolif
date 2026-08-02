QT += quick quickcontrols2

CONFIG += c++17

DEFINES += QT_DEPRECATED_WARNINGS

SOURCES += \
    main.cpp \
    src/revolifcontroller.cpp \
    core/revolif_backend.cpp

HEADERS += \
    src/revolifcontroller.h

RESOURCES += qml.qrc

unix: LIBS += -lcurl
win32: LIBS += -llibcurl

qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
