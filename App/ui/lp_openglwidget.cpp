#include "lp_openglwidget.h"

#include "renderer/lp_glrenderer.h"
#include "renderer/lp_glselector.h"
#include "lp_renderercam.h"
#include "Functionals/lp_functional.h"

#include <QOpenGLContext>
#include <QOpenGLTexture>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QKeyEvent>
#include <QtMath>
#include <QPainter>
#include <QPainterPath>
#include <QDebug>

struct LP_OpenGLWidget::member {
    void    frontView(LP_RendererCam &pCam)
    {
        auto centre = pCam->Target();

        float   zL  = 0.5*pCam->Diagonal()/qTan(qDegreesToRadians(.5f*pCam->FOV()));
        pCam->SetPosition(QVector3D(centre.x(), centre.y(), centre.z() + zL ));
        pCam->SetUp(QVector3D(0.f, 1.f, 0.f));
    }

    void    rightView(LP_RendererCam &pCam)
    {
        auto centre = pCam->Target();

        float   zL  = 0.5*pCam->Diagonal()/qTan(qDegreesToRadians(.5f*pCam->FOV()));
        pCam->SetPosition(QVector3D(centre.x() + zL, centre.y(), centre.z() ));
        pCam->SetUp(QVector3D(0.f, 1.f, 0.f));
    }

    void    topView(LP_RendererCam &pCam)
    {
        auto centre = pCam->Target();

        float   zL  = 0.5*pCam->Diagonal()/qTan(qDegreesToRadians(.5f*pCam->FOV()));
        pCam->SetPosition(QVector3D(centre.x(), centre.y() + zL, centre.z()));
        pCam->SetUp(QVector3D(0.f, 0.f, -1.f));
    }

    void    resetView(LP_RendererCam &pCam)
    {
        pCam->RefreshCamera();
    }

    void    changeProjection(LP_RendererCam &pCam)
    {
        if (pCam->IsPerspective()){
            pCam->SetPerspective(false);
            float   zL  = 0.5*pCam->Diagonal()/qTan(qDegreesToRadians(.5f*pCam->FOV()));
            auto centre = pCam->Target();
            auto camPos = pCam->Position();
            camPos -= centre; camPos.normalize();
            pCam->SetPosition( centre + camPos*zL );
        }
        else{
            pCam->SetPerspective(true);
        }
    }

    void    rotateCam( const float &deg,
                       const QVector3D &vector,
                       LP_RendererCam &pCam)
    {
        QMatrix4x4 tmpT, tmpR;
        tmpT.translate( pCam->Target());
        tmpT.rotate( deg, vector);
        tmpT.translate( -pCam->Target());

        tmpR.rotate( deg, vector);

        QVector3D newPos    = tmpT * pCam->Position();
        QVector3D newUp     = tmpR * pCam->Up();

        pCam->SetPosition( newPos );
        pCam->SetUp( newUp );
    }
};

LP_OpenGLWidget::LP_OpenGLWidget(QWidget *parent) : QOpenGLWidget(parent)
{
    mTexture.load(":/background.jpg");
}

LP_OpenGLWidget::~LP_OpenGLWidget()
{
    makeCurrent();
    delete mBlitter;
    doneCurrent();
    delete mRenderer;
}

void LP_OpenGLWidget::SetRenderer(LP_GLRenderer *renderer)
{
    mRenderer = renderer;
}

void LP_OpenGLWidget::SetRubberBand(QRubberBand *rb)
{
    mRubberBand = rb;
}

LP_GLRenderer *LP_OpenGLWidget::Renderer() const
{
    return mRenderer;
}

void LP_OpenGLWidget::updateTexture(QImage img)
{
    mTexture = img;
    update();
}

void LP_OpenGLWidget::initializeGL()
{
    mBlitter = new QOpenGLTextureBlitter();
    if ( !mBlitter->create()){
        throw std::runtime_error("Display initialization problem : 001");
    }
    context()->doneCurrent();
    emit initGL(context());
}

void LP_OpenGLWidget::resizeGL(int w, int h)
{
    emit reshapeGL(w,h);
}

void LP_OpenGLWidget::paintGL()
{
    constexpr int x(0), y(0);
    QOpenGLTexture tex_(mTexture);
    tex_.setMinMagFilters(QOpenGLTexture::Nearest, QOpenGLTexture::Nearest);
    tex_.create();
    tex_.bind();

    mBlitter->bind();
    const QRect targetRect(QPoint(x, y), QPoint(width(), height()));
    const QMatrix4x4 target = QOpenGLTextureBlitter::targetTransform(targetRect, QRect(QPoint(0, 0), QPoint(width(), height())));
    mBlitter->blit(tex_.textureId(), target, QOpenGLTextureBlitter::OriginTopLeft);
    mBlitter->release();
    tex_.release();

    if (LP_Functional::mCurrentFunctional){
        LP_Functional::mCurrentFunctional->PainterDraw(this);
    }

    auto &cam = mRenderer->mCam;

    QPainter painter(this);
    QPainterPath path;
    float hh = 0.5f*cam->ResolutionY();
    QFont orgFont = painter.font();
    int fontSize(13);
    QFont font("Arial", fontSize);
    QString text;
    if ( cam->IsPerspective()){
        text = "PERSP";
    }else{
        text = "ORTHO";
    }

    painter.setFont(font);
    painter.setPen(QColor(cam->IsPerspective()?Qt::yellow:Qt::white));
    painter.drawText(QPointF(5.f, 2.f*hh-5.f), text );
    painter.setFont(orgFont);
}


void LP_OpenGLWidget::mousePressEvent(QMouseEvent *event)
{
    mCursorPos[0] = event->pos().x();
    mCursorPos[1] = event->pos().y();
    event->ignore();

    mCursorDownPos = event->pos();

    if ( Qt::LeftButton == event->button()){
        mRubberBand->setGeometry(QRect(mCursorDownPos, QSize(2,2)));
        mRubberBand->show();
    }

    QOpenGLWidget::mousePressEvent(event);
}

void LP_OpenGLWidget::mouseReleaseEvent(QMouseEvent *event)
{
    if ( Qt::LeftButton == event->button()){
        QString renderer = objectName() == "window_Shade" ? "Shade" : "Normal";//TODO non-fixed

        if (!(( Qt::ControlModifier | Qt::ShiftModifier)
                & event->modifiers())){
            emit g_GLSelector->ClearSelected();
        }

        //Perform selection
        std::vector<LP_Objectw> objs = g_GLSelector->SelectInWorld(renderer,
                                                                    mRubberBand->pos(),
                                                                    mRubberBand->width(),
                                                                    mRubberBand->height());
//        QMetaObject::invokeMethod(g_GLSelector.get(),
//                                  "SelectInWorld",
//                                  Qt::QueuedConnection,
//                                  Q_RETURN_ARG(LP_Objectw, o),
//                                  Q_ARG(QString, renderer),
//                                  Q_ARG(QPoint, event->pos()));

        if ( !objs.empty()){
            std::vector<QUuid> select, deselect;

            if ( Qt::ShiftModifier & event->modifiers()){
                deselect.resize(objs.size());
                std::transform(objs.cbegin(), objs.cend(), deselect.begin(), [](const LP_Objectw &o){
                    return o.lock()->Uuid();
                });
            }else {
                select.resize(objs.size());
                std::transform(objs.cbegin(), objs.cend(), select.begin(), [](const LP_Objectw &o){
                    return o.lock()->Uuid();
                });
            }

            emit g_GLSelector->Selected(select, deselect);

            //mRenderer->UpdateGL();
            event->accept();
        }
        mRubberBand->hide();
    }
    QOpenGLWidget::mouseReleaseEvent(event);
}


void LP_OpenGLWidget::mouseMoveEvent(QMouseEvent *e)
{
    auto &lock = mRenderer->Lock();
    lock.lockForWrite();
    auto pCam = mRenderer->mCam;

    float   w   = pCam->ResolutionX(),
            h   = pCam->ResolutionY();

     QMatrix4x4 mv(pCam->ViewMatrix());//, proj(pCam->ProjectionMatrix());
     QPoint pos  = e->pos();
     if ((Qt::MiddleButton | Qt::RightButton) & e->buttons()){

         if ( Qt::ControlModifier == e->modifiers()){//Pan
             float   hw   = 0.5*w,
                     hh   = 0.5*h;

             QVector4D np( pos.x()/hw - 1.f, 1.f - pos.y()/hh, 0.f, 1.f );
             QVector4D op( mCursorPos[0]/hw - 1.f, 1.f - mCursorPos[1]/hh, 0.f, 1.f );
             //QMatrix4x4  inv = proj.inverted();
             //QVector3D   t   = (inv * np - inv * op).toVector3D();
             QVector3D   t   = ( np - op).toVector3D();

             pCam->SetPan( pCam->Pan()+t);
         }
         else{//Rotate
             float delta[]   = { float(mCursorPos[0] - pos.x()), float(mCursorPos[1] - pos.y())};
             float angle[]   = { delta[0]/w*360.f, delta[1]/h*360.f};

             QVector4D   vx    = mv.row(0),
                         vy    = mv.row(1);

             m->rotateCam( angle[0], QVector3D(vy), pCam);
             m->rotateCam( angle[1], QVector3D(vx), pCam);
         }

         QMetaObject::invokeMethod(mRenderer,
                                   "updateGL",Qt::QueuedConnection);
     } else if ( Qt::LeftButton & e->buttons()){
         mRubberBand->setGeometry(QRect(mCursorDownPos, pos).normalized());
     }
     mRenderer->Lock().unlock();
     mCursorPos[0]  = pos.x();
     mCursorPos[1]  = pos.y();
}

void LP_OpenGLWidget::wheelEvent(QWheelEvent *e)
{
    auto &lock = mRenderer->Lock();
    lock.lockForWrite();
    auto pCam   = mRenderer->mCam;
    float  // w   = pCam->ResolutionX(),
            h   = pCam->ResolutionY();

    QMatrix4x4 proj( pCam->ProjectionMatrix());
    QMatrix4x4 viewp ( pCam->ViewportMatrix());

    float   l = e->angleDelta().y() / -1440.f;  //Step in 15 degs( a 360 scroll result in a roll with distance pCam->Far() - pCam->Near())
    if (pCam->IsPerspective()){
        QMatrix4x4 view( pCam->ViewMatrix());
        QVector3D camPos    = view.inverted() * QVector3D(0.f,0.f,0.f);
        float   d = camPos.distanceToPoint( pCam->Target());

        if ( Qt::ShiftModifier == e->modifiers()){
            d *= 0.1f;
        }
        l   *= d;

        auto pos  = e->position();
        QVector3D   ndc( proj.inverted()*viewp.inverted()*QVector3D(pos.x(), h-pos.y(), 0.f));

        ndc.normalize();

        pCam->SetRoll( pCam->Roll() + l*ndc );

        //Keep the depth consistant
        view = pCam->ViewMatrix();
        camPos    = view.inverted() * QVector3D(0.f,0.f,0.f);
        d  = camPos.distanceToPoint( pCam->Target());
        float depth = pCam->Far() - pCam->Near();
        float nearP  = qMax(.1f, d - .5f*depth );
        float farP   = nearP + depth;
        pCam->SetNear( nearP );
        pCam->SetFar( farP );

    }
    else{
        if ( Qt::ShiftModifier == e->modifiers()){
            l *= 0.1f;
        }
        l   = qMax(.1f, 1.f + l);
        QVector3D  pos(e->position().x(), h - e->position().y(), 0.f );
        QVector3D   ndc( proj.inverted()*viewp.inverted()*pos);

        float aspect    = pCam->Aspect();

        l   *= pCam->Top();

        pCam->SetTop( l );
        pCam->SetBottom( -l );
        pCam->SetLeft( -l*aspect );
        pCam->SetRight( l*aspect );

        proj    = pCam->ProjectionMatrix();
        QVector3D   nndc( proj.inverted()*viewp.inverted()*pos);
        nndc    -= ndc;
        pCam->SetRoll( pCam->Roll() + nndc );
    }
    lock.unlock();
    e->accept();

    QMetaObject::invokeMethod(mRenderer,
                              "updateGL",Qt::QueuedConnection);
}

void LP_OpenGLWidget::keyPressEvent(QKeyEvent *e)
{
    auto &lock = mRenderer->Lock();
    lock.lockForWrite();

    e->ignore();
    switch (e->key()) {
    case Qt::Key_0:
        if (Qt::KeypadModifier == e->modifiers()){
            //TODO: Revise this ugly design
            QMetaObject::invokeMethod(mRenderer,
                                      &LP_GLRenderer::updateTarget,
                                      Qt::BlockingQueuedConnection);
            m->resetView( mRenderer->mCam );
            QMetaObject::invokeMethod(mRenderer,
                                      "updateGL",Qt::QueuedConnection);
            e->accept();
        }
        break;
    case Qt::Key_1:
        if (Qt::KeypadModifier == e->modifiers()){
            m->frontView(mRenderer->mCam);
            QMetaObject::invokeMethod(mRenderer,
                                      "updateGL",Qt::QueuedConnection);
            e->accept();
        }
        break;
    case Qt::Key_2:
        if (Qt::KeypadModifier & e->modifiers()){
            auto pCam   = mRenderer->mCam;
            QMatrix4x4 view(pCam->ViewMatrix());
            float angle(15.f);

            m->rotateCam(angle, QVector3D(view.row(0)), pCam);
            QMetaObject::invokeMethod(mRenderer,
                                      "updateGL",Qt::QueuedConnection);
            e->accept();
        }
        break;
    case Qt::Key_3:
        if (Qt::KeypadModifier == e->modifiers()){
            m->rightView(mRenderer->mCam);
            QMetaObject::invokeMethod(mRenderer,
                                      "updateGL",Qt::QueuedConnection);
            e->accept();
        }
        break;
    case Qt::Key_4:
        if (Qt::KeypadModifier == e->modifiers()){
            float angle(-15.f);
            if ( Qt::ShiftModifier == e->modifiers()){
                angle *= 0.2f;
            }
            m->rotateCam(angle, QVector3D(0.f,1.f,0.f),mRenderer->mCam);
            QMetaObject::invokeMethod(mRenderer,
                                      "updateGL",Qt::QueuedConnection);
            e->accept();
        }
        break;
    case Qt::Key_5:
        if (Qt::KeypadModifier == e->modifiers()){
            m->changeProjection( mRenderer->mCam );
            QMetaObject::invokeMethod(mRenderer,
                                      "updateGL",Qt::QueuedConnection);
            e->accept();
        }
        break;
    case Qt::Key_6:
        if (Qt::KeypadModifier == e->modifiers()){
            float angle(15.f);
            if ( Qt::ShiftModifier == e->modifiers()){
                angle *= 0.2f;
            }
            m->rotateCam(angle, QVector3D(0.f,1.f,0.f), mRenderer->mCam );
            QMetaObject::invokeMethod(mRenderer,
                                      "updateGL",Qt::QueuedConnection);
            e->accept();
        }
        break;
    case Qt::Key_7:
        if (Qt::KeypadModifier == e->modifiers()){
            m->topView( mRenderer->mCam );
            QMetaObject::invokeMethod(mRenderer,
                                      "updateGL",Qt::QueuedConnection);
            e->accept();
        }
    case Qt::Key_8:
        if (Qt::KeypadModifier == e->modifiers()){
            auto pCam   = mRenderer->mCam;
            QMatrix4x4 view(pCam->ViewMatrix());
            float angle(-15.f);
            if ( Qt::ShiftModifier == e->modifiers()){
                angle *= 0.2f;
            }
            m->rotateCam(angle, QVector3D(view.row(0)),mRenderer->mCam);
            QMetaObject::invokeMethod(mRenderer,
                                      "updateGL",Qt::QueuedConnection);
            e->accept();
        }
        break;
    default: {
            const auto &selObj = g_GLSelector->Objects();
            if ( selObj.isEmpty()){
                break;
            }
            switch (e->key()) {
            case Qt::Key_G:
                qDebug() << cursor().pos() - pos() - window()->pos()
                         << window()->pos();
                //Start the move geometry functional
                break;
            default:
                e->ignore();
                break;
            }
            break;
        }
    }
    lock.unlock();
    if (e->isAccepted()){
        return;
    }
    QOpenGLWidget::keyPressEvent(e);
}

void LP_OpenGLWidget::keyReleaseEvent(QKeyEvent *event)
{
    event->ignore();
    QOpenGLWidget::keyReleaseEvent(event);
}


void LP_OpenGLWidget::focusInEvent(QFocusEvent *event)
{
    auto frame = qobject_cast<QWidget*>(parent());
    frame->setStyleSheet("QFrame{"
                         "border:1px solid gold;"
                         "}");
    event->ignore();
    QOpenGLWidget::focusInEvent(event);
}

void LP_OpenGLWidget::focusOutEvent(QFocusEvent *event)
{
    auto frame = qobject_cast<QWidget*>(parent());
    frame->setStyleSheet("QFrame{"
                         "border:1px solid transparent;"
                         "}");
    event->ignore();
    QOpenGLWidget::focusOutEvent(event);
}
