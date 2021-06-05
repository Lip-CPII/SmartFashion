#include "lp_glrenderer.h"
#include "lp_glselector.h"

#include "lp_renderercam.h"

#include "lp_document.h"
#include "lp_geometry.h"

#include <cmath>
#include <memory>

#include <QOpenGLExtraFunctions>
#include <QOpenGLContext>
#include <QOpenGLFramebufferObject>
#include <QOpenGLShaderProgram>
#include <QOffscreenSurface>

QMap<QString, LP_GLRenderer*> g_Renderers;

LP_GLRenderer::LP_GLRenderer(QObject *parent) : QObject(parent)
  ,mContext(nullptr)
  ,mFBO(nullptr)
{
    mSurface = std::make_shared<QOffscreenSurface>();
    mSurface->create();
    moveToThread(&mThread);
    mThread.start();
    mCam = LP_RendererCamImpl::Create(nullptr);
    mCam->SetPerspective(true);
}

LP_GLRenderer *LP_GLRenderer::Create(const QString &name)
{
    auto it = g_Renderers.find(name);
    if ( it != g_Renderers.cend()){
        return it.value();
    }
    auto renderer = new LP_GLRenderer;
    renderer->setObjectName(name);
    g_Renderers[name] = renderer;
    return renderer;
}

void LP_GLRenderer::UpdateAll()
{
    for ( auto &r : g_Renderers){
        r->UpdateGL();
    }
}

LP_GLRenderer::~LP_GLRenderer()
{
    if ( QThread::currentThread() == &mThread ){
        clearScene();
        destroyGL();
    }else{
        QMetaObject::invokeMethod(this, "clearScene", Qt::BlockingQueuedConnection);
        QMetaObject::invokeMethod(this, "destroyGL", Qt::BlockingQueuedConnection);
    }
    mThread.quit();
    if (!mThread.wait(5000)){
        mThread.terminate();
    }
}

QReadWriteLock &LP_GLRenderer::Lock()
{
    return mLock;
}


void LP_GLRenderer::UpdateGL(bool blocking)
{
    if ( QThread::currentThread() == &mThread ){
        qDebug() << "Dirrect update";
        updateGL();
    }else{
        QMetaObject::invokeMethod(this,"updateGL",
                                  blocking? Qt::BlockingQueuedConnection:Qt::QueuedConnection);
    }
}

void LP_GLRenderer::GLContextRequest(LP_GLRenderer::EmptyCB _cb, QString name)
{
    auto it = g_Renderers.find(name);
    if ( it == g_Renderers.end()){
        qDebug() << "No such renderer : " << name;
        return;
    }
    QMetaObject::invokeMethod(*it, "glContextResponse",
                              Qt::BlockingQueuedConnection,
                              Q_ARG(EmptyCB, _cb));
}

void LP_GLRenderer::UpdateGL_By_Name(QString name)
{
    if ( QString("All") == name){
        for ( auto r : qAsConst(g_Renderers) ){
            QMetaObject::invokeMethod(r, "updateGL",
                                      Qt::QueuedConnection);
        }
        return;
    }
    auto it = g_Renderers.find(name);
    if ( it == g_Renderers.end()){
        qDebug() << "No such renderer : " << name;
        return;
    }
    QMetaObject::invokeMethod(*it, "updateGL",
                              Qt::QueuedConnection);
}

void LP_GLRenderer::initGL(QOpenGLContext *context)
{
    Q_ASSERT(mSurface);

    mContext = new QOpenGLContext;
    mContext->setFormat(context->format());
    mContext->setShareContext(context);

    if ( !mContext->create()){
        throw std::runtime_error("Create context failed");
    }

    if ( !mContext->makeCurrent(mSurface.get())){
        throw std::runtime_error("Context make current failed");
    }

    mContext->doneCurrent();

    g_GLSelector->InitialGL(mContext, mSurface.get());
}

void LP_GLRenderer::reshapeGL(int w, int h)
{
    mLock.lockForWrite();
    mContext->makeCurrent(mSurface.get());

    if ( mFBO ){
        delete mFBO;
        mFBO = nullptr;
    }

    QOpenGLFramebufferObjectFormat fmt;
    fmt.setSamples(4);
    fmt.setAttachment(QOpenGLFramebufferObject::CombinedDepthStencil);

    auto scale = 1;//0.5*mSample;
    mFBO = new QOpenGLFramebufferObject(w*scale,h*scale,fmt);

    mContext->doneCurrent();

    mCam->ResizeFilm(w,h);

    mLock.unlock();
    updateGL();
}

void LP_GLRenderer::updateGL()
{
    auto pDoc = &LP_Document::gDoc;
    auto &dLock = pDoc->Lock();

    mLock.lockForWrite();
    mContext->makeCurrent(mSurface.get());
    auto f = mContext->extraFunctions();
    f->glViewport(0,0,mFBO->width(), mFBO->height());
    f->glClearColor(0.6f, 0.6f, 0.6f, 1.0f);

    mFBO->bind();

    f->glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT );
    f->glEnable( GL_DEPTH_TEST );

    QVariant option = QVariant::fromValue(objectName());
    //Renderer all objects in the doc
    dLock.lockForRead();

    for ( auto &o : pDoc->Objects()){
        o.lock()->Draw(mContext, mSurface.get(), mFBO, mCam, option);
    }

    f->glDisable( GL_DEPTH_TEST );
    g_GLSelector->DrawLabel(mContext, mSurface.get(), mFBO, mCam );
    f->glEnable( GL_DEPTH_TEST );

    dLock.unlock();

    emit postRender(mContext, mSurface.get(), mFBO, mCam, QVariant(objectName()));

    f->glDisable( GL_DEPTH_TEST );
    mFBO->release();

    mTexture = mFBO->toImage();
    mContext->doneCurrent();

    mLock.unlock();

    emit textureUpdated(mTexture);
}

void LP_GLRenderer::clearScene()
{
    Q_ASSERT(QThread::currentThread() == &mThread); //Done not call clearScene() directly

    auto pDoc = &LP_Document::gDoc;

    for ( auto &o : pDoc->Objects()){
        o.lock()->DrawCleanUp(mContext, mSurface.get());
    }
}

void LP_GLRenderer::destroyObjectGL(LP_Objectw o)
{
    //Make sure the object is not longer in gDoc
    g_GLSelector->_removeObject(o); //Remove if selected
    o.lock()->DrawCleanUp(mContext, mSurface.get());
    updateTarget();
    updateGL();
}

void LP_GLRenderer::updateTarget()
{
    Q_ASSERT(QThread::currentThread() == &mThread);

    QVector3D bbmin({std::numeric_limits<float>::max(),
                    std::numeric_limits<float>::max(),
                    std::numeric_limits<float>::max()}),
            bbmax({-std::numeric_limits<float>::max(),
                  -std::numeric_limits<float>::max(),
                  -std::numeric_limits<float>::max()});

    auto pDoc = &LP_Document::gDoc;
    auto &dLock = pDoc->Lock();
    dLock.lockForRead();

    auto targets = &g_GLSelector->Objects();
    if ( targets->empty()){
        targets = &pDoc->Objects();
    }
    for ( auto &o : *targets){
        LP_Geometryw geo = std::static_pointer_cast<LP_GeometryImpl>(o.lock());
        if ( auto g = geo.lock()){
            auto trans = g->ModelTrans();
            QVector3D bmin, bmax;
            g->BoundingBox(bmin, bmax);
            bmin = trans * bmin;
            bmax = trans * bmax;
            bbmin.setX( qMin(bbmin.x(),bmin.x()));
            bbmin.setY( qMin(bbmin.y(),bmin.y()));
            bbmin.setZ( qMin(bbmin.z(),bmin.z()));

            bbmax.setX( qMax(bbmax.x(),bmax.x()));
            bbmax.setY( qMax(bbmax.y(),bmax.y()));
            bbmax.setZ( qMax(bbmax.z(),bmax.z()));
        }
    }

    dLock.unlock();

    auto diag = (bbmax - bbmin).length();
    if ( std::isinf(diag)){
        return;
    }
    auto center = 0.5*(bbmax + bbmin);

    mCam->setDiagonal(diag);
    mCam->SetTarget(center);
    mCam->RefreshCamera();
}

void LP_GLRenderer::glContextResponse(LP_GLRenderer::EmptyCB _cb)
{
    Q_ASSERT(QThread::currentThread() == &mThread);
    mContext->makeCurrent(mSurface.get());
    _cb();
    mContext->doneCurrent();
}

void LP_GLRenderer::initObjectGL(LP_Objectw o)
{
    QVariant option = QVariant::fromValue(objectName());
    o.lock()->DrawSetup(mContext, mSurface.get(), option);
    updateTarget();
    updateGL();
}

void LP_GLRenderer::bindWidget(LP_OpenGLWidget *widget)
{
    if (!widget){
        return;
    }
}

void LP_GLRenderer::destroyGL()
{
    mContext->makeCurrent(mSurface.get());

    delete mFBO;
    mFBO = nullptr;

    mContext->doneCurrent();

    g_GLSelector->DestroyGL(mContext, mSurface.get());
    mContext->deleteLater();
}
