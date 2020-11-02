#ifndef LP_YOLO_HELPER_H
#define LP_YOLO_HELPER_H

#include "lp_functional.h"
#include "opencv2/opencv.hpp"
#include <QReadWriteLock>
#include <QThreadPool>
#include <QVector3D>
#include <QVector2D>

class QOpenGLBuffer;
class QComboBox;
class QOpenGLShaderProgram;

class LP_YOLO_Helper : public LP_Functional
{
    Q_OBJECT
    REGISTER_FUNCTIONAL
public:
    explicit LP_YOLO_Helper(QObject *parent = nullptr);

    // LP_Functional interface
    QWidget *DockUi() override;
    bool Run() override;

    // QObject interface
    bool eventFilter(QObject *watched, QEvent *event) override;

signals:

public slots:
    // LP_Functional interface
    void PainterDraw(QWidget *glW) override;
    void FunctionalRender(QOpenGLContext *ctx, QSurface *surf, QOpenGLFramebufferObject *fbo, const LP_RendererCam &cam, const QVariant &options) override;

    void exportYOLOset();

private:
    bool mbGreyScale = false;
    bool mbCrtlDown = false;
    bool mInitialized = false;
    std::shared_ptr<QWidget> mWidget;
    QComboBox *mClasses;
    QOpenGLShaderProgram *mProgram = nullptr;
    QOpenGLBuffer *mVBO;
    QOpenGLBuffer *mIndices;
    QImage mImage;
    QString mVideoFile;
    std::shared_ptr<cv::VideoCapture> mCVCam;
    double mCurrentTime = 0.0;
    double mTotalTime = 0.0;
    int mFBOHeight = 0;
    QWidget *mGLWidget = nullptr;
    std::weak_ptr<LP_RendererCamImpl> mCam;
    QReadWriteLock mLock;
    QThreadPool mPool;
    struct YOLO_BoundingBox {
        std::pair<QVector2D,QVector2D> mPickPoints;
        std::pair<QVector3D,QVector3D> mPickPoints3D;
        std::vector<QVector3D> mRect;
        short mClass;
    };
    std::vector<YOLO_BoundingBox> mBoundingBoxesYOLO;
    std::pair<QVector2D, QVector2D> mCurrentBoundingBox;

    /**
     * @brief initializeGL initalize any OpenGL resource
     */
    void initializeGL();




};

#endif // LP_YOLO_HELPER_H
