#ifndef LP_OPENGLWIDGET_H
#define LP_OPENGLWIDGET_H

#include <QOpenGLWidget>
#include <QOpenGLTextureBlitter>
class LP_GLRenderer;

class LP_OpenGLWidget : public QOpenGLWidget
{
    Q_OBJECT
public:
    explicit LP_OpenGLWidget(QWidget *parent = nullptr);
    virtual ~LP_OpenGLWidget();

    LP_GLRenderer *Renderer() const;
    void SetRenderer(LP_GLRenderer* renderer);


public slots:
    void updateTexture(QImage img);

signals:
    void initGL(QOpenGLContext *);
    void reshapeGL(int,int);

    // QOpenGLWidget interface
protected:
    void initializeGL() override;
    void resizeGL(int w, int h) override;
    void paintGL() override;

    // QWidget interface
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *e) override;
    void wheelEvent(QWheelEvent *e) override;
    void keyPressEvent(QKeyEvent *event) override;
    void keyReleaseEvent(QKeyEvent *event) override;

private:
    QOpenGLTextureBlitter *mBlitter;
    LP_GLRenderer *mRenderer;
    QImage mTexture;
    float mCursorPos[2];
    struct member;
    member *m;
};

#endif // LP_OPENGLWIDGET_H
