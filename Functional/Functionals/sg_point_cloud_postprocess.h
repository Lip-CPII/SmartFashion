
#ifndef SG_POINT_CLOUD_POSTPROCESS_H
#define SG_POINT_CLOUD_POSTPROCESS_H

#include "lp_functional.h"
#include "extern/geodesic/geodesic_algorithm_exact.h"
#include <opencv2/opencv.hpp>
#include <QReadWriteLock>
#include <QThreadPool>
#include <QVector3D>
#include <QVector2D>
#include <QWaitCondition>
#include <QFutureWatcher>
#include <QDoubleSpinBox>
#include "pcl/io/pcd_io.h" //PCL的PCD格式文件的输入输出头文件
#include "pcl/io/ply_io.h"
#include "pcl/point_types.h" //PCL对各种格式的点的支持头文件
//#include "pcl/visualization/cloud_viewer.h"//点云查看窗口头文件
#include "pcl/common/time.h"
#include "pcl/filters/statistical_outlier_removal.h"
#include "pcl/filters/voxel_grid.h"
#include "pcl/filters/radius_outlier_removal.h"
//#include "pcl/filters/fast_bilateral.h"
#include <QObject>
class QOpenGLBuffer;
class QComboBox;
class QLabel;
class QSlider;
class QOpenGLShaderProgram;
class LP_ObjectImpl;

class sg_point_cloud_postprocess : public LP_Functional
{
    Q_OBJECT
    REGISTER_FUNCTIONAL
public:
    explicit sg_point_cloud_postprocess(QObject *parent = nullptr);

    // LP_Functional interface
    QWidget *DockUi() override;
    bool Run() override;
    bool eventFilter(QObject *watched, QEvent *event) override;
    bool importPLY();
    bool resample();
    bool filter_ROR();
    bool filter_statistic();
    bool remove_manual();
public slots:
    void FunctionalRender_L(QOpenGLContext *ctx, QSurface *surf, QOpenGLFramebufferObject *fbo, const LP_RendererCam &cam, const QVariant &options) override;
private:
    QDoubleSpinBox *resample_x = new QDoubleSpinBox();
    QDoubleSpinBox *resample_y = new QDoubleSpinBox();
    QDoubleSpinBox *resample_z = new QDoubleSpinBox();
    QDoubleSpinBox *ROR_p1 = new QDoubleSpinBox();
    QDoubleSpinBox *ROR_p2 = new QDoubleSpinBox();
    QDoubleSpinBox *statistic_p1 = new QDoubleSpinBox();
    QDoubleSpinBox *statistic_p2 = new QDoubleSpinBox();
    std::vector<pcl::PointCloud<pcl::PointXYZRGB>::Ptr> cloud_buffer;
    std::vector<int > indexs_selected;
    std::shared_ptr<QWidget> mWidget;
    QLabel *labelBufferLength=nullptr;
    QOpenGLShaderProgram *mProgram = nullptr;
    std::weak_ptr<LP_ObjectImpl> mObject;
    std::weak_ptr<LP_RendererCamImpl> mCam;
    bool mInitialized= false;
    std::vector<QVector3D> mVs;
    std::vector<QVector3D> mNs;
    std::vector<QVector3D> mCs;
    void initializeGL();
signals:
};


#endif // SG_POINT_CLOUD_POSTPROCESS_H
