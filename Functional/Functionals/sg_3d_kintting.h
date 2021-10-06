#ifndef SG_3D_KINTTING_H
#define SG_3D_KINTTING_H

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

#include <QObject>

//#include "lp_import_openmesh.h"
class QOpenGLBuffer;
class QComboBox;
class QLabel;
class QSlider;
class QOpenGLShaderProgram;
class LP_ObjectImpl;




class SG_3D_Kintting : public LP_Functional
{
    Q_OBJECT
    REGISTER_FUNCTIONAL //THIS IS REQUIRED FOR ANY LP_Functional-DERIVED CLASS
public:
    explicit SG_3D_Kintting(QObject *parent = nullptr);
    virtual ~SG_3D_Kintting();
    QWidget *DockUi() override;
    bool Run() override;
    bool eventFilter(QObject *watched, QEvent *event) override;
    void ResetSelection();
    bool comDistanceField();
    bool isoCurveGeneration();
    bool courseGeneration();
    bool knittingMapGeneration();
signals:

    // LP_Functional interface
public slots:

    void FunctionalRender_L(QOpenGLContext *ctx, QSurface *surf, QOpenGLFramebufferObject *fbo, const LP_RendererCam &cam, const QVariant &options) override;
    void PainterDraw(QWidget *glW) override;
private:
    bool test = false;
    bool fieldMode = false;
    bool selectMode = false;
    bool mInitialized= false;
    std::shared_ptr<QWidget> mWidget;
    QDoubleSpinBox *mDis;
    QDoubleSpinBox *mDis_course;
    QImage mImage;
    QLabel *mLabel = nullptr;
    QOpenGLShaderProgram *mProgram = nullptr;
    std::weak_ptr<LP_ObjectImpl> mObject;
    std::weak_ptr<LP_RendererCamImpl> mCam;
    QMap<uint,uint> mPoints;
    std::vector<double> Points;
    std::vector<unsigned> Faces;
    std::vector<float> point_distance;
    float maxDis;
    std::vector<float> field_color;
    std::vector<float> ocolor;
    QLabel *labelMaxDis;
    std::vector<QMap<uint,uint>> oriEdgeSet;
    std::vector<std::vector<QMap<uint,uint>>> isoEdgeSet;
    std::vector<std::vector<std::vector<float>>>oriEdgePoint;
    std::vector<std::vector<float>>oriEdgeRatio;
    std::vector<std::vector<QMap<uint,uint>>>isoNodeSequence;
    std::vector<std::vector<std::vector<std::vector<float>>>>isoCurveNode;
    std::vector<std::vector<std::vector<std::vector<float>>>>isoCurveNode_resampled;
    std::vector<std::vector<std::vector<std::vector<std::vector<int>>>>> relatedNode;
    std::vector<std::vector<std::vector<std::vector<std::vector<int>>>>> relatedNode_assist;
    std::vector<std::vector<std::vector<float>>> firstNormal;
    bool isoCurveMode = false;
    bool knittingMapMode = false;
    void initializeGL();
};

#endif // SG_3D_KINTTING_H
