#ifndef LP_GEODESIC_H
#define LP_GEODESIC_H

#include <QObject>
#include "lp_functional.h"

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

        void FunctionalRender(QOpenGLContext *ctx, QSurface *surf, QOpenGLFramebufferObject *fbo, const LP_RendererCam &cam, const QVariant &options) override;

private:
        bool mInitialized = false;
        std::shared_ptr<QWidget> mWidget;
        QLabel *mLabel = nullptr;
        QOpenGLShaderProgram *mProgram = nullptr;
        std::weak_ptr<LP_ObjectImpl> mObject;
        std::vector<uint> mPoints;
        std::vector<uint> mFirstPoint;
        std::vector<uint> mSecondPoint;

private:
        /**
         * @brief initializeGL initalize any OpenGL resource
         */
void initializeGL();
};

#endif // LP_GEODESIC_H
