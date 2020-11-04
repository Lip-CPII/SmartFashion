#ifndef LP_GLRENDERER_H
#define LP_GLRENDERER_H

#include "Functional_global.h"
#include <QThread>
#include <QReadWriteLock>
#include <QImage>
#include <QOpenGLContext>
#include <QOffscreenSurface>
#include <QOpenGLFramebufferObject>

class LP_OpenGLWidget;
//class QSurface;
//class QOpenGLContext;
//class QOpenGLFramebufferObject;
//class QOffscreenSurface;
class QOpenGLShaderProgram;
class LP_RendererCamImpl;
typedef std::shared_ptr<LP_RendererCamImpl> LP_RendererCam;

class LP_ObjectImpl;
typedef std::weak_ptr<LP_ObjectImpl> LP_Objectw;

typedef std::function<void(QOpenGLContext*,QSurface*,QOpenGLFramebufferObject*, const LP_RendererCam &, void*)> LP_GLRendererCB;

class FUNCTIONAL_EXPORT LP_GLRenderer : public QObject
{
    Q_OBJECT
public:
    typedef std::function<void()> EmptyCB;

    static LP_GLRenderer* Create(const QString &name);
    static void UpdateAll();

    virtual ~LP_GLRenderer();

    QReadWriteLock &Lock();
    LP_RendererCam mCam;

    /**
     * @brief RunInContext
     * @param cb
     */
    Q_INVOKABLE void RunInContext(LP_GLRendererCB cb, void *data = nullptr){
        Q_ASSERT(QThread::currentThread() == &mThread);
        if (mLock.tryLockForWrite(2000)){
            cb(mContext, mSurface.get(), mFBO, mCam, data);
            mLock.unlock();
        }else{
            throw std::runtime_error("To busy to Handle the task");
        }
    }

    /**
     * @brief UpdateGL for external call to update
     */
    void UpdateGL(bool blocking = false);

public slots:
    void initGL(QOpenGLContext *context);
    void reshapeGL(int w, int h);
    void updateGL();
    void clearScene();
    void initObjectGL(LP_Objectw o);
    void destroyObjectGL(LP_Objectw o);
    void updateTarget();
    void glContextResponse(EmptyCB _cb);

signals:
    void textureUpdated(QImage);
    void postRender(QOpenGLContext*, QSurface*, QOpenGLFramebufferObject*,
                    const LP_RendererCam&, const QVariant &);

protected:
    explicit LP_GLRenderer(QObject *parent = nullptr );
    void bindWidget(LP_OpenGLWidget *widget);

private:
    QThread mThread;
    QImage mTexture;
    QOpenGLContext *mContext;
    QOpenGLFramebufferObject *mFBO;
    std::shared_ptr<QOffscreenSurface> mSurface;
    mutable QReadWriteLock mLock;

    Q_INVOKABLE void destroyGL();
};

extern QMap<QString, LP_GLRenderer*> g_Renderers;

#endif // LP_GLRENDERER_H
