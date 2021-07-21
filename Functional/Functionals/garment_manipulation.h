#ifndef GARMENT_MANIPULATION_H
#define GARMENT_MANIPULATION_H

#include <QObject>
#include "lp_functional.h"
#include "extern/geodesic/geodesic_algorithm_exact.h"
#include <QOpenGLBuffer>
#include <QCheckBox>

#include "opencv2/aruco.hpp"
#include "opencv2/aruco/charuco.hpp"
#include "opencv2/imgcodecs.hpp"
#include "opencv2/highgui.hpp"
#include "opencv2/imgproc.hpp"
#include "opencv2/videoio.hpp"
#include "opencv2/opencv.hpp"
#include "opencv2/calib3d.hpp"

#include <librealsense2/rs.hpp>


#include <math.h>
#include <QProcess>
#include <QFile>
#include <QTextStream>
#include <QThread>

class QLabel;
class LP_ObjectImpl;
class QOpenGLShaderProgram;

/**
 * @brief The Garment_Manipulation class
 * Ros2 TM robot arm garment manipulation
 */


class Garment_Manipulation : public LP_Functional
{
        Q_OBJECT
        REGISTER_FUNCTIONAL
public:
    explicit Garment_Manipulation(QObject *parent = nullptr);
    virtual ~Garment_Manipulation();

        // LP_Functional interface
        QWidget *DockUi() override;
        bool Run() override;
        bool eventFilter(QObject *watched, QEvent *event) override;
        bool saveCameraParams(const std::string &filename, cv::Size imageSize, float aspectRatio, int flags,
                                     const cv::Mat &cameraMatrix, const cv::Mat &distCoeffs, double totalAvgErr);

signals:


        // LP_Functional interface
public slots:

        void FunctionalRender_L(QOpenGLContext *ctx, QSurface *surf, QOpenGLFramebufferObject *fbo, const LP_RendererCam &cam, const QVariant &options) override;
        void FunctionalRender_R(QOpenGLContext *ctx, QSurface *surf, QOpenGLFramebufferObject *fbo, const LP_RendererCam &cam, const QVariant &options) override;
        void calibrate();
        void Robot_Plan(int, void* );

private:
        bool mInitialized_L = false;
        bool mInitialized_R = false;
        std::shared_ptr<QWidget> mWidget;
        QLabel *mLabel = nullptr;
        QOpenGLShaderProgram *mProgram_L = nullptr,
                             *mProgram_R = nullptr;


        // Realsense configuration structure, it will define streams that need to be opened
//        rs2::config cfg;

        rs2::pipeline pipe;


        // Frames returned by our pipeline, they will be packed in this structure
        rs2::frameset frames;

        std::vector<QVector3D> mPointCloud, mPointCloudColor, mPointCloudCopy, mTestP;
        std::vector<QVector2D> mPointCloudTex;

private:
        /**
         * @brief initializeGL initalize any OpenGL resource
         */
void initializeGL_L();
void initializeGL_R();
};

#endif // GARMENT_MANIPULATION_H
