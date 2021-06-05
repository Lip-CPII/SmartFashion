#ifndef LP_POSEESTIMATION_H
#define LP_POSEESTIMATION_H

#include "lp_functional.h"

class LP_PoseEstimation : public LP_Functional
{
    Q_OBJECT
    REGISTER_FUNCTIONAL
public:
    explicit LP_PoseEstimation(LP_Functional *parent = nullptr);
    virtual ~LP_PoseEstimation();

signals:


    // QObject interface
public:
    bool eventFilter(QObject *watched, QEvent *event);

    // LP_Functional interface
public:
    QWidget *DockUi();
    bool Run();

public slots:
    void FunctionalRender_L(QOpenGLContext *ctx, QSurface *surf, QOpenGLFramebufferObject *fbo, const LP_RendererCam &cam, const QVariant &options);
    void PainterDraw(QWidget *glW);

protected:
    Q_INVOKABLE void Estimate(const QImage &img );

private:
    bool mGrab;
    QImage mImage;
    struct member;
    member *m;
};

#endif // LP_POSEESTIMATION_H
