#ifndef LP_PICK_FEATURE_POINTS_OFF_H
#define LP_PICK_FEATURE_POINTS_OFF_H


#include <QObject>
#include "lp_functional.h"

class QLabel;
class LP_ObjectImpl;
class QOpenGLShaderProgram;

/**
 * @brief The LP_Pick_Feature_Points class
 * Pick some feature points from an LP_OpenMesh's
 * verties
 */
class LP_Pick_Feature_Points_OFF : public LP_Functional
{
    Q_OBJECT
    REGISTER_FUNCTIONAL //THIS IS REQUIRED FOR ANY LP_Functional-DERIVED CLASS
public:
    explicit LP_Pick_Feature_Points_OFF(QObject *parent = nullptr);
    virtual ~LP_Pick_Feature_Points_OFF();

    // LP_Functional interface
    QWidget *DockUi() override;
    bool Run() override;

    /**
     * @brief eventFilter should be implemented if custom 3D
     * interaction is required. E.g. Picking vertices on a
     * LP_OpenMesh.
     * @param watched the 3D View
     * @param event is the event triggered in the 3D view,
     * e.g. mouse-click, key-press etc.
     * @return true if a particular even is captured and don't
     * want any other widget/object to handle it. Else, just
     * pass to the QObject::eventFilter().
     */
    bool eventFilter(QObject *watched, QEvent *event) override;
signals:


    // LP_Functional interface
public slots:
    /**
     * @brief FunctionalRender
     * @param ctx is the OpenGL context owned by the 3D view
     * @param surf is a device dependent surface for renderering
     * @param fbo is the framebuffer which is finally shown on the screen
     * @param cam is the viewing camera of the 3D view
     * @param options reserved option
     *
     * Please reference the implementation as an example of usage.
     */
    void FunctionalRender_L(QOpenGLContext *ctx, QSurface *surf, QOpenGLFramebufferObject *fbo, const LP_RendererCam &cam, const QVariant &options) override;
    void PainterDraw(QWidget *glW) override;

private:
    bool mInitialized = false;
    std::shared_ptr<QWidget> mWidget;
    QLabel *mLabel = nullptr;
    QOpenGLShaderProgram *mProgram = nullptr;
    std::vector<QVector3D> mVs;
    std::vector<uint> mFids;
    std::vector<uint> mEdges;
    std::weak_ptr<LP_RendererCamImpl> mCam;
    QMap<uint,uint> mPoints;

private:
    /**
     * @brief initializeGL initalize any OpenGL resource
     */
    void initializeGL();
};

#endif // LP_PICK_FEATURE_POINTS_OFF_H
