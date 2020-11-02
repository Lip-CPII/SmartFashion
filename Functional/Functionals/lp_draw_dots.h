#ifndef LP_DRAW_DOTS_H
#define LP_DRAW_DOTS_H


#include "lp_functional.h"

class LP_ObjectImpl;
class QOpenGLBuffer;
class QSpinBox;
class QSlider;
class QOpenGLShaderProgram;

class LP_Draw_Dots : public LP_Functional
{
    Q_OBJECT
    REGISTER_FUNCTIONAL
public:
    explicit LP_Draw_Dots(QObject *parent = nullptr);
    virtual ~LP_Draw_Dots();

    // LP_Functional interface
    QWidget *DockUi() override;
    bool Run() override;


    // QObject interface
    bool eventFilter(QObject *watched, QEvent *event) override;

public slots:
    void FunctionalRender(QOpenGLContext *ctx, QSurface *surf, QOpenGLFramebufferObject *fbo, const LP_RendererCam &cam, const QVariant &options) override;


signals:

private:
    bool mInitialized;
    std::shared_ptr<QWidget> mWidget;
    QOpenGLShaderProgram *mProgram = nullptr;
    QOpenGLBuffer *mVBO;
    QOpenGLBuffer *mIndices;
    QSpinBox *mR, *mG, *mB;
    QSlider *mSlider;
    QImage mImage;

    /**
     * @brief initializeGL initalize any OpenGL resource
     */
    void initializeGL();

};

#endif // LP_DRAW_DOTS_H
