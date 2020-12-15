#ifndef LP_MESH_SLICER_H
#define LP_MESH_SLICER_H


#include "plugin/lp_actionplugin.h"
#include <QFutureWatcher>
#include <QReadWriteLock>
#include <QThreadPool>
#include <QVector3D>

/*Define the name, version of the plugin
*/
#define LP_Mesh_Slicer_Plugin_iid "cpii.rp5.SmartFashion.LP_Mesh_Slicer/0.1"

class QLabel;
class QSlider;
class LP_ObjectImpl;
class QOpenGLShaderProgram;

/**
 * @brief The LP_Mesh_Slicer class
 * This plugin provides functions that slice an LP_OpenMesh with angled plane
 */
class LP_Mesh_Slicer : public LP_ActionPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID LP_Mesh_Slicer_Plugin_iid)
    Q_INTERFACES(LP_ActionPlugin)
public:
    virtual ~LP_Mesh_Slicer();

    // LP_ActionPlugin interface
    QString MenuName() override;
    QWidget *DockUi() override;
    bool Run() override;
    QAction *Trigger() override;

    // QObject interface
    bool eventFilter(QObject *watched, QEvent *event) override;

public slots:
    void FunctionalRender(QOpenGLContext *ctx, QSurface *surf, QOpenGLFramebufferObject *fbo,
                                const LP_RendererCam &cam, const QVariant &options) override;

private:
    bool mMouseMove = false;
    bool mInitialized = false;
    std::once_flag mFlag1;
    std::shared_ptr<QWidget> mWidget;
    QLabel *mLabel = nullptr;
    float mRange[2] = {0,0};
    QOpenGLShaderProgram *mProgram = nullptr;
    std::weak_ptr<LP_ObjectImpl> mObject;
    QVector3D mNormal{0.f,1.0f,0.f};
//    QVector3D mNormal{0.f,0.0f,1.f};
    QFutureWatcher<void> mWatcher;
    QMap<int, std::vector<std::vector<QVector3D>>> mPaths;
    QReadWriteLock mLock;
    QThreadPool mPool;

    void initializeGL();

private slots:
    void sliderValueChanged(int v);
};

#endif // LP_MESH_SLICER_H
