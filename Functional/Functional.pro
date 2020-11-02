QT += gui widgets

TEMPLATE = lib
DEFINES += FUNCTIONAL_LIBRARY _USE_MATH_DEFINES

CONFIG += c++17

win32{
QMAKE_CXXFLAGS+= -openmp
}
unix {
QMAKE_CXXFLAGS += -fopenmp
QMAKE_LFLAGS += -fopenmp
}


# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    Commands/lp_cmd_addentity.cpp \
    Commands/lp_cmd_import_opmesh.cpp \
    Commands/lp_command.cpp \
    Commands/lp_commandmanager.cpp \
    Functionals/lp_draw_dots.cpp \
    Functionals/lp_file_open.cpp \
    Functionals/lp_file_save.cpp \
    Functionals/lp_functional.cpp \
    Functionals/lp_import_openmesh.cpp \
    Functionals/lp_new.cpp \
    Functionals/lp_pick_feature_points.cpp \
    Functionals/lp_yolo_helper.cpp \
    plugin/lp_actionplugin.cpp \
    renderer/lp_glrenderer.cpp \
    renderer/lp_glselector.cpp

HEADERS += \
    Commands/lp_cmd_addentity.h \
    Commands/lp_cmd_import_opmesh.h \
    Commands/lp_command.h \
    Commands/lp_commandmanager.h \
    Functional_global.h \
    Functionals/lp_draw_dots.h \
    Functionals/lp_file_open.h \
    Functionals/lp_file_save.h \
    Functionals/lp_functional.h \
    Functionals/lp_functionalregistry.h \
    Functionals/lp_import_openmesh.h \
    Functionals/lp_new.h \
    Functionals/lp_pick_feature_points.h \
    Functionals/lp_yolo_helper.h \
    LP_Registry.h \
    plugin/lp_actionplugin.h \
    renderer/lp_glrenderer.h \
    renderer/lp_glselector.h

# Default rules for deployment.
unix {
    target.path = /usr/lib
}

!isEmpty(target.path): INSTALLS += target



win32:CONFIG(release, debug|release): LIBS += -L$$PWD/../../../OpenMesh/lib/ -lOpenMeshCore
else:win32:CONFIG(debug, debug|release): LIBS += -L$$PWD/../../../OpenMesh/lib/ -lOpenMeshCored
else:unix:!macx: LIBS += -L$$PWD/../../../OpenMesh/lib/ -lOpenMeshCore

INCLUDEPATH += $$PWD/../../../OpenMesh/include
DEPENDPATH += $$PWD/../../../OpenMesh/include

win32:CONFIG(release, debug|release): LIBS += -L$$OUT_PWD/../Model/release/ -lModel
else:win32:CONFIG(debug, debug|release): LIBS += -L$$OUT_PWD/../Model/debug/ -lModel
else:unix:!macx: LIBS += -L$$OUT_PWD/../Model/ -lModel

INCLUDEPATH += $$PWD/../Model
DEPENDPATH += $$PWD/../Model

win32:CONFIG(release, debug|release): LIBS += -L$$PWD/../../../../../OpenCV/4_3_0/install/release/x64/vc16/lib/ \
    -lopencv_core430 \
    -lopencv_videoio430 \
    -lopencv_imgproc430

else:win32:CONFIG(debug, debug|release): LIBS += -L$$PWD/../../../../../OpenCV/4_3_0/install/debug/x64/vc16/lib/ \
    -lopencv_core430d \
    -lopencv_videoio430d \
    -lopencv_imgproc430d

else:unix:!macx: LIBS += -L$$PWD/../../OpenCV/install/lib/  \
    -lopencv_core \
    -lopencv_videoio \
    -lopencv_imgproc

INCLUDEPATH += $$PWD/../../OpenCV/install/include/opencv4
DEPENDPATH += $$PWD/../../OpenCV/install/include/opencv4

win32:CONFIG(release, debug|release): LIBS += -L$$PWD/../../OpenMesh/lib/release/ -lOpenMeshCore
else:win32:CONFIG(debug, debug|release): LIBS += -L$$PWD/../../OpenMesh/lib/debug/ -lOpenMeshCore
else:unix:!macx: LIBS += -L$$PWD/../../OpenMesh/lib/ -lOpenMeshCore

INCLUDEPATH += $$PWD/../../OpenMesh/include
DEPENDPATH += $$PWD/../../OpenMesh/include
