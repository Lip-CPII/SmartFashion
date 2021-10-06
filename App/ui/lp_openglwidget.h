#ifndef LP_OPENGLWIDGET_H
#define LP_OPENGLWIDGET_H

#include <QOpenGLWidget>
#include <QOpenGLTextureBlitter>
#include <QRubberBand>

class LP_GLRenderer;

class LP_OpenGLWidget : public QOpenGLWidget
{
    Q_OBJECT
public:
    explicit LP_OpenGLWidget(QWidget *parent = nullptr);
    virtual ~LP_OpenGLWidget();

    LP_GLRenderer *Renderer() const;
    void SetRenderer(LP_GLRenderer* renderer);

    void SetRubberBand(QRubberBand *rb);

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
    void focusInEvent(QFocusEvent *event) override;
    void focusOutEvent(QFocusEvent *event) override;

private:
    QOpenGLTextureBlitter *mBlitter;
    LP_GLRenderer *mRenderer;
    QRubberBand *mRubberBand;
    QImage mTexture;
    int mDownKey = 0;
    float mCursorPos[2];
    QPoint mCursorDownPos;
    struct member;
    member *m;
};

#endif // LP_OPENGLWIDGET_H
