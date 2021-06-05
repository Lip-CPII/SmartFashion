#ifndef LP_GEODESIC_H
#define LP_GEODESIC_H

#include <QObject>
#include "lp_functional.h"
#include "extern/geodesic/geodesic_algorithm_exact.h"
#include <QOpenGLBuffer>
#include <QCheckBox>


class QLabel;
class LP_ObjectImpl;
class QOpenGLShaderProgram;

/**
 * @brief The LP_Geodesic class
 * Pick two points from an LP_OpenMesh's verties
 * and generate a geodesic path between them
 * then compute distance field of the path
 */

class LP_Geodesic : public LP_Functional
{
        Q_OBJECT
        REGISTER_FUNCTIONAL
public:
    explicit LP_Geodesic(QObject *parent = nullptr);
    virtual ~LP_Geodesic();

        // LP_Functional interface
        QWidget *DockUi() override;
        bool Run() override;
        bool eventFilter(QObject *watched, QEvent *event) override;
signals:


        // LP_Functional interface
public slots:

        void FunctionalRender_L(QOpenGLContext *ctx, QSurface *surf, QOpenGLFramebufferObject *fbo, const LP_RendererCam &cam, const QVariant &options) override;
        void FunctionalRender_R(QOpenGLContext *ctx, QSurface *surf, QOpenGLFramebufferObject *fbo, const LP_RendererCam &cam, const QVariant &options) override;


private:
        bool mInitialized_L = false;
        bool mInitialized_R = false;
        bool geocolored = false;
        std::shared_ptr<QWidget> mWidget;
        QLabel *mObjectid = nullptr;
        QLabel *mLabel = nullptr;
        QCheckBox *geocheckbox = nullptr;
        QOpenGLShaderProgram *mProgram_L = nullptr,
                             *mProgram_R = nullptr;
        std::weak_ptr<LP_ObjectImpl> mObject;
        std::vector<uint> mFirstPoint;
        std::vector<uint> mSecondPoint;
        std::vector<double> Points;
        std::vector<unsigned> Faces;
        std::vector<float> path_point;
        std::vector<float> point_distance;
        float length;
        std::vector<float> field_color;
        std::vector<float> ocolor;


private:
        /**
         * @brief initializeGL initalize any OpenGL resource
         */
void initializeGL_L();
void initializeGL_R();
};

#endif // LP_GEODESIC_H
