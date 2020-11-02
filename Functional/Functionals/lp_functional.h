#ifndef LP_FUNCTIONAL_H
#define LP_FUNCTIONAL_H

#include <QObject>

#include "lp_functionalregistry.h"  //TODO Generalize to LP_Registry


class QAction;
class QOpenGLContext;
class QSurface;
class QOpenGLFramebufferObject;


class LP_RendererCamImpl;
typedef std::shared_ptr<LP_RendererCamImpl> LP_RendererCam;

/**
 * @brief The LP_Functional class
 * Every functional is a Contexted function that interact with
 * the GUI and data within the whole application.
 * A sub-GUI can be provide by implementing the DockUi() which will
 * be shown in the "Context UI" tab when a functional is triggered.
 * It is not neccessary to provide a sub-GUI, e.g. New, Import, Save etc.
 * Run() MUST BE IMPLEMENTED FOR EVERY FUNCTIONAL.
 * Functional should generate LP_COMMANDS and save to the system by
 * pushing the generated command(s) onto the History stack,
 * e.g. LP_COMMANDSTACK::Push().
 * Custom 3D objects/handles can be draw by implementing the
 * FunctionalRenderer() using OpenGL shaders. Interaction with the
 * OpenGL renderer by emit glUpdateRequest() or destroy the
 * initialized OpenGL resource by glContextRequest() with a
 * callback.
 */
class FUNCTIONAL_EXPORT LP_Functional : public QObject
{
    Q_OBJECT

    /**
     * Destructor callback should be in a simple form
     */
    using GLDestroy_CB = std::function<void()>;

public:
    explicit LP_Functional(QObject *parent = nullptr):QObject(parent),mAction(nullptr){}
    virtual ~LP_Functional(){};

    /**
     * @brief DockUi
     * @return A QWidget that contains the sub-GUI for this
     * functional
     */
    virtual QWidget *DockUi(){return nullptr;}

    /**
     * @brief Run execute the functional. A functional is not
     * necessarily monotonic, e.g. No intermediate interaction
     * with the main application content, like New, Import, Open etc.
     * If they are monotonic, only Run() is needed while DockUi() and
     * FunctionalRender() can be left without implementation.
     * @return true if execution is successful.
     */
    virtual bool Run() = 0;

    inline static LP_Functional *mCurrentFunctional = nullptr;
    inline static void ClearCurrent(){
        delete LP_Functional::mCurrentFunctional;
        LP_Functional::mCurrentFunctional = nullptr;
    }
    inline static void SetCurrent(LP_Functional *f){
        LP_Functional::mCurrentFunctional = f;
    }

public slots:
    virtual void FunctionalRender(QOpenGLContext *ctx, QSurface *surf, QOpenGLFramebufferObject *fbo,
                              const LP_RendererCam &cam, const QVariant &options){
        Q_UNUSED(ctx);
        Q_UNUSED(surf);
        Q_UNUSED(cam);
        Q_UNUSED(fbo);
        Q_UNUSED(options);
    };
    virtual void PainterDraw(QWidget *glW){
        Q_UNUSED(glW);
    };

protected:
    QAction *mAction;

signals:
    /**
     * @brief glUpdateRequest request for a screen update
     * if something is added/require-display in/to the 3D scene
     * by this functional. E.g. mesh vertices in LP_Pick_Feature_Point.
     */
    void glUpdateRequest();

    /**
     * @brief glContextRequest Mostly be used in destructor
     * for cleaning up and OpenGL resources created in
     * FunctionalRender()
     */
    void glContextRequest(GLDestroy_CB);
};


#endif // LP_FUNCTIONAL_H
