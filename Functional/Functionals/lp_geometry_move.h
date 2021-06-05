#ifndef LP_GEOMETRY_MOVE_H
#define LP_GEOMETRY_MOVE_H

#include "lp_functional.h"
#include <QMatrix4x4>

class QOpenGLShaderProgram;
class LP_GeometryImpl;

class LP_Geometry_Move : public LP_Functional
{
    Q_OBJECT
    REGISTER_FUNCTIONAL
public:
    explicit LP_Geometry_Move(QObject *parent = nullptr);
    virtual ~LP_Geometry_Move();

signals:
    // QObject interface
public:
    bool eventFilter(QObject *watched, QEvent *event);
    // LP_Functional interface
public:
    bool Run();

private:
    bool mInitialized;

    QMatrix4x4 mTransform;
    QPoint mCursorPos;
    std::weak_ptr<LP_GeometryImpl> mGeo;
    short mAxis;        //0 = X, 1 = Y, 2 = Z
    QOpenGLShaderProgram *mProgram;

private:
    /**
     * @brief initializeGL initalize any OpenGL resource
     */
    void initializeGL();

    // LP_Functional interface
public slots:
    void FunctionalRender_L(QOpenGLContext *ctx, QSurface *surf, QOpenGLFramebufferObject *fbo, const LP_RendererCam &cam, const QVariant &options);
};

#endif // LP_GEOMETRY_MOVE_H
