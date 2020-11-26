QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17 console

VERSION = 0.0.0.1


# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    lp_application.cpp \
    main.cpp \
    lp_mainwindow.cpp \
    ui/lp_openglwidget.cpp \
    ui/lp_progressbar.cpp

HEADERS += \
    lp_application.h \
    lp_mainwindow.h \
    ui/lp_openglwidget.h \
    ui/lp_progressbar.h

FORMS += \
    lp_mainwindow.ui

TRANSLATIONS += \
    App_zh_HK.ts

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

RESOURCES += \
    images/images.qrc

win32:CONFIG(release, debug|release): LIBS += -L$$OUT_PWD/../Model/release/ -lModel
else:win32:CONFIG(debug, debug|release): LIBS += -L$$OUT_PWD/../Model/debug/ -lModel
else:unix:!macx: LIBS += -L$$OUT_PWD/../Model/ -lModel

INCLUDEPATH += $$PWD/../Model
DEPENDPATH += $$PWD/../Model

win32:CONFIG(release, debug|release): LIBS += -L$$OUT_PWD/../Functional/release/ -lFunctional
else:win32:CONFIG(debug, debug|release): LIBS += -L$$OUT_PWD/../Functional/debug/ -lFunctional
else:unix:!macx: LIBS += -L$$OUT_PWD/../Functional/ -lFunctional

INCLUDEPATH += $$PWD/../Functional
DEPENDPATH += $$PWD/../Functional

win32:CONFIG(release, debug|release): LIBS += -L$$PWD/../../OpenCV/install/release/bin \
    -L$$PWD/../../OpenMesh/lib

else:win32:CONFIG(debug, debug|release): LIBS += -L$$PWD/../../OpenCV/install/debug/bin \
    -L$$PWD/../../OpenMesh/lib

unix:!macx: LIBS += -L$$PWD/../../OpenCV/install/lib/  \
    -lopencv_core \
    -lopencv_videoio \
    -lopencv_imgproc \
    -lopencv_imgcodecs

unix:!macx: LIBS += -L$$PWD/../../OpenMesh/lib/ -lOpenMeshCore
