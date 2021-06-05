#ifndef LP_PLUGIN_PIFUHD_H
#define LP_PLUGIN_PIFUHD_H

#include "LP_Plugin_PIFuHD_global.h"

#include "plugin/lp_actionplugin.h"
#include <QFutureWatcher>
#include <QReadWriteLock>
#include <QThreadPool>
#include <QVector3D>

#define LP_PIFu_HD_Plugin_iid "cpii.rp5.SmartFashion.LP_Plugin_PIFuHD/0.1"

class QLabel;
class QSlider;
class LP_ObjectImpl;
class QOpenGLShaderProgram;


class LP_PLUGIN_PIFUHD_EXPORT LP_Plugin_PIFuHD : public LP_ActionPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID LP_PIFu_HD_Plugin_iid)
    Q_INTERFACES(LP_ActionPlugin)
public:
    virtual ~LP_Plugin_PIFuHD();

    // LP_Functional interface
    QWidget *DockUi() override;
    bool Run() override;

    // LP_ActionPlugin interface
    QString MenuName() override;
    QAction *Trigger() override;

public slots:
    void FunctionalRender_L(QOpenGLContext *ctx, QSurface *surf, QOpenGLFramebufferObject *fbo, const LP_RendererCam &cam, const QVariant &options) override;

private:
    bool mInitialized = false;
    std::shared_ptr<QWidget> mWidget;
    QOpenGLShaderProgram *mProgram = nullptr;
    QReadWriteLock mLock;
    QThreadPool mPool;

    void initializeGL();
};

#endif // LP_PLUGIN_PIFUHD_H
