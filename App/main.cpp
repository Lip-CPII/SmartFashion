#include "lp_mainwindow.h"
#include "lp_application.h"

#include <QSurfaceFormat>
#include <QDebug>

int main(int argc, char *argv[])
{
    QSurfaceFormat fmt;
    fmt.setSamples(4);
    QSurfaceFormat::setDefaultFormat(fmt);
    QCoreApplication::setAttribute(Qt::AA_ShareOpenGLContexts);

    LP_Application a(argc, argv);
    LP_MainWindow w;
    w.show();

    return a.exec();
}
