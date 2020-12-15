#include "lp_object.h"
#include <QVariant>

LP_ObjectImpl::LP_ObjectImpl(LP_Objectw parent)
    :mParent(parent)
{
    mUid = QUuid::createUuid();
    if ( auto p = mParent.lock()){
       p->mChildren.insert(shared_from_this());
    }
}


LP_ObjectImpl::~LP_ObjectImpl()
{

}

bool LP_ObjectImpl::DrawSetup(QOpenGLContext *ctx, QSurface *surf, QVariant &option)
{
    Q_UNUSED(ctx)
    Q_UNUSED(surf)

    Q_UNUSED(option)
    return false;
}

void LP_ObjectImpl::Draw(QOpenGLContext *ctx, QSurface *surf, QOpenGLFramebufferObject *fbo, const LP_RendererCam &cam, QVariant &option)
{
    Q_UNUSED(ctx)
    Q_UNUSED(surf)
    Q_UNUSED(cam)
    Q_UNUSED(fbo)
    Q_UNUSED(option)
    Q_ASSERT_X(0,__FILE__,"Please override Draw(...) for display etc.");
}

void LP_ObjectImpl::DrawSelect(QOpenGLContext *ctx, QSurface *surf, QOpenGLFramebufferObject *fbo, QOpenGLShaderProgram *prog, const LP_RendererCam &cam)
{
    Q_UNUSED(ctx)
    Q_UNUSED(surf)
    Q_UNUSED(prog)
    Q_UNUSED(cam)
    Q_UNUSED(fbo)
    Q_ASSERT_X(0,__FILE__,"Please override DrawSelect(...) for selection (Pure geometry without any shading).");
}

bool LP_ObjectImpl::DrawCleanUp(QOpenGLContext *ctx, QSurface *surf)
{
    Q_UNUSED(ctx)
    Q_UNUSED(surf)
    return false;
}

QUuid LP_ObjectImpl::Uuid() const
{
    return mUid;
}

void LP_ObjectImpl::SetUuid(const QUuid &id)
{
    mUid = id;
}
