CONFIG += core

TEMPLATE = lib
DEFINES += MODEL_LIBRARY _USE_MATH_DEFINES

CONFIG += c++17

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    lp_document.cpp \
    lp_geometry.cpp \
    lp_model.cpp \
    lp_object.cpp \
    lp_openmesh.cpp \
    lp_pointcloud.cpp \
    lp_renderercam.cpp

HEADERS += \
    Model_global.h \
    lp_document.h \
    lp_geometry.h \
    lp_model.h \
    lp_object.h \
    lp_openmesh.h \
    lp_pointcloud.h \
    lp_renderercam.h

# Default rules for deployment.
unix {
    target.path = /usr/lib
}
!isEmpty(target.path): INSTALLS += target

win32:CONFIG(release, debug|release): LIBS += -L$$PWD/../../OpenMesh/lib/ -lOpenMeshCore
else:win32:CONFIG(debug, debug|release): LIBS += -L$$PWD/../../OpenMesh/lib/ -lOpenMeshCored
else:unix:!macx: LIBS += -L$$PWD/../../OpenMesh/lib/ -lOpenMeshCore

INCLUDEPATH += $$PWD/../../OpenMesh/include
DEPENDPATH += $$PWD/../../OpenMesh/include

win32: LIBS += -L$$PWD/../../opennurbs/lib/ -lopennurbs_public
else:unix:!macx: LIBS += -L$$PWD/../../opennurbs/lib/ -lopennurbs_public

INCLUDEPATH += $$PWD/../../opennurbs/include
DEPENDPATH += $$PWD/../../opennurbs/include
