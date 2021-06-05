#include "lp_geometry_move.h"
#include "renderer/lp_glselector.h"
#include "renderer/lp_glrenderer.h"
#include "lp_renderercam.h"

#include "lp_geometry.h"

#include "Commands/lp_cmd_transform.h"

#include "Commands/lp_commandmanager.h"

#include <QKeyEvent>
#include <QMouseEvent>
#include <QOpenGLShaderProgram>
#include <QOpenGLExtraFunctions>


REGISTER_FUNCTIONAL_IMPLEMENT(LP_Geometry_Move, Move Geometry, menuGeometry)

LP_Geometry_Move::LP_Geometry_Move(QObject *parent) : LP_Functional(parent)
  ,mInitialized(false)
  ,mAxis(0)
  ,mProgram(nullptr)
{

}

LP_Geometry_Move::~LP_Geometry_Move()
{
    emit glContextRequest([this](){
        delete mProgram;
        mProgram = nullptr;
    });
    Q_ASSERT(!mProgram);
}

bool LP_Geometry_Move::eventFilter(QObject *watched, QEvent *event)
{
    event->ignore();
    auto widget = qobject_cast<QWidget*>(watched);
    auto h = widget->height();

    auto v_ = g_Renderers.find("Shade");
    if ( g_Renderers.cend() == v_ ){
        qDebug() << "Unknown renderer : Shade";
        return true;
    }
    auto renderer = v_.value();

    if ( QEvent::MouseMove == event->type()){
        auto e = static_cast<QMouseEvent*>(event);
        auto geo = mGeo.lock();
        if ( Qt::LeftButton == e->buttons() && geo){
            auto deltaPos = e->pos() - mCursorPos;
            deltaPos.setY( h - deltaPos.y());

            renderer->Lock().lockForRead();

            const auto &cam = renderer->mCam;
            auto view = cam->ViewMatrix(),
                 proj = cam->ProjectionMatrix(),
                 viewport = cam->ViewportMatrix();

            renderer->Lock().unlock();
            view = viewport * proj * view;
            view = view.inverted();

            auto &trans = mTransform;
            QVector3D xVec_pre(mCursorPos.x(), h - mCursorPos.y(), 0.0f);
            QVector3D xVec(e->pos().x(), h - e->pos().y(), 0.0f);
            xVec = view * xVec;
            xVec_pre = view * xVec_pre;
            trans.translate(xVec - xVec_pre);

            mCursorPos = e->pos();
            emit glUpdateRequest();
            e->accept();
        }
    } else if (QEvent::MouseButtonPress == event->type()) {
        auto e = static_cast<QMouseEvent*>(event);
        auto geo = mGeo.lock();
        if ( geo && Qt::RightButton == e->button()){
            auto cmd = new LP_Cmd_Transform;
            cmd->SetTrans(mTransform);
            cmd->SetGeometry(geo->Uuid().toString());
            if ( !cmd->VerifyInputs()){
                delete cmd;
            }else{
                LP_CommandGroup::gCommandGroup->ActiveStack()->Push(cmd);   //Execute the command
                emit glUpdateRequest();
            }

            mGeo.reset();
            mTransform = QMatrix4x4();
        }else{//Select the geometry
            if ( Qt::LeftButton == e->button() && !geo ){
                auto &&objs = g_GLSelector->SelectInWorld("Shade",e->pos());
                for ( auto &o : objs ){
                    auto geo = std::static_pointer_cast<LP_GeometryImpl>(o.lock());
                    if ( geo ){
                        mGeo = geo;
                        mCursorPos = e->pos();
                        mTransform = geo->ModelTrans();
                        emit glUpdateRequest();
                        return true;    //Since event filter has been called
                    }
                }
            }
        }
        mCursorPos = e->pos();
    }else if (QEvent::MouseButtonRelease == event->type()){
        return true;
    }else if ( QEvent::KeyPress == event->type()){
        auto e = static_cast<QKeyEvent*>(event);
        switch (e->key()) {
        case Qt::Key_X:
            mAxis = 0;
            e->accept();
            break;
        case Qt::Key_Y:
            mAxis = 1;
            e->accept();
            break;
        case Qt::Key_Z:
            mAxis = 2;
            e->accept();
            break;
        default:
            e->ignore();
            break;
        }
    }
    if ( event->isAccepted()){
        return true;
    }
    return QObject::eventFilter(watched, event);
}

bool LP_Geometry_Move::Run()
{
    emit glUpdateRequest();
    return false;
}

void LP_Geometry_Move::initializeGL()
{

    constexpr char vsh[] =
            "attribute vec3 a_pos;\n"
            "attribute vec3 a_norm;\n"
            "attribute vec3 a_color;\n"
            "attribute vec2 a_tex;\n"
            "uniform mat4 m4_mvp;\n"        //The Model-View-Matrix
            "void main(){\n"
            "   gl_Position = m4_mvp * vec4(a_pos, 1.0);\n" //Output the OpenGL position
            "}";
    constexpr char fsh[] =
            "uniform vec4 v4_color;\n"       //Defined the point color variable that will be set in FunctionRender()
            "void main(){\n"
            "   gl_FragColor = v4_color;\n" //Output the fragment color;
            "}";

    auto prog = new QOpenGLShaderProgram;   //Intialize the Shader with the above GLSL codes
    prog->addShaderFromSourceCode(QOpenGLShader::Vertex,vsh);
    prog->addShaderFromSourceCode(QOpenGLShader::Fragment,fsh);
    if (!prog->create() || !prog->link()){  //Check whether the GLSL codes are valid
        qDebug() << prog->log();
        return;
    }

    mProgram = prog;            //If everything is fine, assign to the member variable

    mInitialized = true;
}

void LP_Geometry_Move::FunctionalRender_L(QOpenGLContext *ctx, QSurface *surf, QOpenGLFramebufferObject *fbo, const LP_RendererCam &cam, const QVariant &options)
{
    if ( !mInitialized ){
        initializeGL();
    }
    auto geo = mGeo.lock();
    if ( !geo ){
        return;
    }

    auto f = ctx->extraFunctions();
    f->glEnable( GL_DEPTH_TEST );

    f->glCullFace(GL_BACK);
    f->glEnable(GL_CULL_FACE);
    f->glEnable(GL_BLEND);
    f->glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    f->glDepthFunc(GL_LEQUAL);

    fbo->bind();

    auto orgTrans = geo->ModelTrans();
    geo->SetModelTrans(mTransform);

    mProgram->bind();
    mProgram->setUniformValue("v4_color", QVector4D(0.3f, 0.8f, 0.9f, 0.7f));

    geo->DrawSelect(ctx, surf, fbo, mProgram, cam);
    mProgram->release();

    fbo->release();

    geo->SetModelTrans(orgTrans);

    f->glDisable( GL_BLEND );
    f->glDisable( GL_DEPTH_TEST );
    f->glDisable( GL_CULL_FACE );
    f->glDepthFunc(GL_LESS);
}
