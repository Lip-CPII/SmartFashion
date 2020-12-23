QT += gui widgets concurrent

TEMPLATE = lib
DEFINES += FUNCTIONAL_LIBRARY _USE_MATH_DEFINES

CONFIG += c++17

unix {
QMAKE_CXXFLAGS += -fopenmp
QMAKE_LFLAGS += -fopenmp
LIBS += -fopenmp
}

QMAKE_POST_LINK=$(MAKE) install

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    Commands/lp_cmd_addentity.cpp \
    Commands/lp_cmd_import_opmesh.cpp \
    Commands/lp_cmd_transform.cpp \
    Commands/lp_command.cpp \
    Commands/lp_commandmanager.cpp \
    Functionals/lp_draw_dots.cpp \
    Functionals/lp_file_open.cpp \
    Functionals/lp_file_save.cpp \
    Functionals/lp_functional.cpp \
    Functionals/lp_geodesic.cpp \
    Functionals/lp_geometry_move.cpp \
    Functionals/lp_import_openmesh.cpp \
    Functionals/lp_new.cpp \
    Functionals/lp_pick_feature_points.cpp \
    Functionals/lp_pick_feature_points_off.cpp \
    Functionals/lp_poseestimation.cpp \
    Functionals/lp_surfnet.cpp \
    Functionals/lp_yolo_helper.cpp \
    extern/Geometry.cpp \
    extern/LBO.cpp \
    extern/Mesh.cpp \
    plugin/lp_actionplugin.cpp \
    renderer/lp_glrenderer.cpp \
    renderer/lp_glselector.cpp

HEADERS += \
    Commands/lp_cmd_addentity.h \
    Commands/lp_cmd_import_opmesh.h \
    Commands/lp_cmd_transform.h \
    Commands/lp_command.h \
    Commands/lp_commandmanager.h \
    Functional_global.h \
    Functionals/lp_draw_dots.h \
    Functionals/lp_file_open.h \
    Functionals/lp_file_save.h \
    Functionals/lp_functional.h \
    Functionals/lp_functionalregistry.h \
    Functionals/lp_geodesic.h \
    Functionals/lp_geometry_move.h \
    Functionals/lp_import_openmesh.h \
    Functionals/lp_new.h \
    Functionals/lp_pick_feature_points.h \
    Functionals/lp_pick_feature_points_off.h \
    Functionals/lp_poseestimation.h \
    Functionals/lp_surfnet.h \
    Functionals/lp_yolo_helper.h \
    LP_Registry.h \
    extern/Geometry.h \
    extern/LBO.h \
    extern/Mesh.h \
    extern/geodesic/geodesic_algorithm_base.h \
    extern/geodesic/geodesic_algorithm_dijkstra.h \
    extern/geodesic/geodesic_algorithm_exact.h \
    extern/geodesic/geodesic_algorithm_exact_elements.h \
    extern/geodesic/geodesic_algorithm_graph_base.h \
    extern/geodesic/geodesic_algorithm_subdivision.h \
    extern/geodesic/geodesic_constants.h \
    extern/geodesic/geodesic_mesh.h \
    extern/geodesic/geodesic_mesh_elements.h \
    plugin/lp_actionplugin.h \
    renderer/lp_glrenderer.h \
    renderer/lp_glselector.h


# Runtime files
runtimes.path = $$OUT_PWD/../App/runtimes
runtimes.files += "runtimes/*"
INSTALLS += runtimes


win32:CONFIG(release, debug|release): LIBS += -L$$OUT_PWD/../Model/release/ -lModel
else:win32:CONFIG(debug, debug|release): LIBS += -L$$OUT_PWD/../Model/debug/ -lModel
else:unix:!macx: LIBS += -L$$OUT_PWD/../Model/ -lModel

INCLUDEPATH += $$PWD/../Model
DEPENDPATH += $$PWD/../Model

win32:CONFIG(release, debug|release): {
    LIBS += -L$$PWD/../../OpenCV/install/release/lib/ \
       -lopencv_core450 \
       -lopencv_videoio450 \
       -lopencv_imgproc450 \
       -lopencv_imgcodecs450 \
       -lopencv_dnn450

    INCLUDEPATH += $$PWD/../../OpenCV/install/release/include
    DEPENDPATH += $$PWD/../../OpenCV/install/release/include
}
else:win32:CONFIG(debug, debug|release): {
    LIBS += -L$$PWD/../../OpenCV/install/debug/lib/ \
       -lopencv_core450d \
       -lopencv_videoio450d \
       -lopencv_imgproc450d \
       -lopencv_imgcodecs450d \
       -lopencv_dnn450d

    INCLUDEPATH += $$PWD/../../OpenCV/install/debug/include
    DEPENDPATH += $$PWD/../../OpenCV/install/debug/include
}
else:unix:!macx: {
    LIBS += -L$$PWD/../../OpenCV/install/lib/  \
        -lopencv_core \
        -lopencv_videoio \
        -lopencv_imgproc \
        -lopencv_imgcodecs \
        -lopencv_dnn

    INCLUDEPATH += $$PWD/../../OpenCV/install/include/opencv4
    DEPENDPATH += $$PWD/../../OpenCV/install/include/opencv4
}

win32:CONFIG(release, debug|release): LIBS += -L$$PWD/../../OpenMesh/lib/ -lOpenMeshCore
else:win32:CONFIG(debug, debug|release): LIBS += -L$$PWD/../../OpenMesh/lib/ -lOpenMeshCored
else:unix:!macx: LIBS += -L$$PWD/../../OpenMesh/lib/ -lOpenMeshCore

INCLUDEPATH += $$PWD/../../OpenMesh/include
DEPENDPATH += $$PWD/../../OpenMesh/include

win32:CONFIG(release, debug|release): {
PATH += $$PWD/../../OpenCV/install/release/bin
INCLUDEPATH += $$PWD/../../OpenCV/install/release/bin
DEPENDPATH += $$PWD/../../OpenCV/install/release/bin
}
else:win32:CONFIG(debug, debug|release): {
LIBS += -L$$PWD/../../OpenCV/install/debug/bin
INCLUDEPATH += $$PWD/../../OpenCV/install/debug/bin
DEPENDPATH += $$PWD/../../OpenCV/install/debug/bin
}


