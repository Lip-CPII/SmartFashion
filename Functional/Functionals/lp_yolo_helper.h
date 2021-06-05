#ifndef LP_YOLO_HELPER_H
#define LP_YOLO_HELPER_H

#include "lp_functional.h"
#include <opencv2/opencv.hpp>
#include <QReadWriteLock>
#include <QThreadPool>
#include <QVector3D>
#include <QVector2D>
#include <QWaitCondition>
#include <QFutureWatcher>

class QOpenGLBuffer;
class QComboBox;
class QLabel;
class QSlider;
class QOpenGLShaderProgram;

class LP_YOLO_Helper : public LP_Functional
{
    Q_OBJECT
    REGISTER_FUNCTIONAL
public:
    explicit LP_YOLO_Helper(QObject *parent = nullptr);
    virtual ~LP_YOLO_Helper();

    // LP_Functional interface
    QWidget *DockUi() override;
    bool Run() override;

    // QObject interface
    bool eventFilter(QObject *watched, QEvent *event) override;

signals:

public slots:
    // LP_Functional interface
    void PainterDraw(QWidget *glW) override;
    void FunctionalRender_L(QOpenGLContext *ctx, QSurface *surf, QOpenGLFramebufferObject *fbo, const LP_RendererCam &cam, const QVariant &options) override;

    void exportYOLOset();

private:
    bool mStop = false;
    bool mPause = false;
    bool mbGreyScale = false;
    bool mbCrtlDown = false;
    bool mInitialized = false;
    std::shared_ptr<QWidget> mWidget;
    QComboBox *mClasses;
    QSlider *mSlider;
    QOpenGLShaderProgram *mProgram = nullptr;
    QOpenGLBuffer *mVBO;
    QOpenGLBuffer *mIndices;
    QImage mImage;
    QString mVideoFile;
    QString mExportPath;
    std::shared_ptr<cv::VideoCapture> mCVCam;
    double mCurrentTime = 0.0;
    double mTotalTime = 0.0;
    int mFBOHeight = 0;
    QWidget *mGLWidget = nullptr;
    QLabel *mLabel = nullptr;
    std::weak_ptr<LP_RendererCamImpl> mCam;
    QReadWriteLock mLock;
    QThreadPool mPool;
    QWaitCondition mWait;
    QFutureWatcher<void> mWatcher;
    struct YOLO_BoundingBox {
        std::pair<QVector2D,QVector2D> mPickPoints;
        std::pair<QVector3D,QVector3D> mPickPoints3D;
        std::vector<QVector3D> mRect;
        short mClass;

        /**
         * @brief hit
         * @param pos
         * @return true is the "pos" hits the top-left corner
         */
        static bool hit(const QPoint &pos, const YOLO_BoundingBox &box, std::weak_ptr<LP_RendererCamImpl> camw);
    };
    std::vector<YOLO_BoundingBox> mBoundingBoxesYOLO;
    std::pair<QVector2D, QVector2D> mCurrentBoundingBox;

    /**
     * @brief initializeGL initalize any OpenGL resource
     */
    void initializeGL();




};

#endif // LP_YOLO_HELPER_H
