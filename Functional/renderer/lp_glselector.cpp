#include "lp_glselector.h"
#include "renderer/lp_glrenderer.h"

#include "lp_renderercam.h"
#include "lp_document.h"

#include <cmath>

#include <QOpenGLContext>
#include <QOpenGLFramebufferObject>
#include <QOpenGLExtraFunctions>
#include <QOpenGLShaderProgram>
#include <QtConcurrent/QtConcurrent>

std::unique_ptr<LP_GLSelector> g_GLSelector = std::make_unique<LP_GLSelector>();

//This cause 192MB stays in memory for performance
std::vector<QVector3D> LP_GLSelector::g_24ColorVector(LP_GLSelector::gen24ColorVector(std::pow(2,24)));

LP_GLSelector::LP_GLSelector()
{

}

std::vector<QVector3D> LP_GLSelector::gen24ColorVector(const int &limit)
{
    std::vector<QVector3D> _c(limit);
    auto watcher = new QFutureWatcher<void>;
    connect(watcher,&QFutureWatcher<void>::finished,
            [watcher](){
        watcher->deleteLater();
    });
    auto future = QtConcurrent::map(_c,std::bind([]( QVector3D &c, QVector3D *base){
        constexpr float denom = 1.0f/255.0f;
        const auto i = &c - base;
        c.setX(((i & 0x000000FF) >>  0)*denom);
        c.setY(((i & 0x0000FF00) >>  8)*denom);
        c.setZ(((i & 0x00FF0000) >>  16)*denom);
    }, std::placeholders::_1, _c.data()));

    watcher->setFuture(future);
    return _c;
}

void LP_GLSelector::_appendObject(const LP_Objectw &o)
{
    QWriteLocker locker(&mLock);
    mSelectedObjs.insert(o);
}

void LP_GLSelector::_removeObject(const LP_Objectw &o)
{
    QWriteLocker locker(&mLock);
    mSelectedObjs.remove(o);
}

void LP_GLSelector::_clear()
{
    QWriteLocker locker(&mLock);
    mSelectedObjs.clear();
}

const QSet<LP_Objectw> &LP_GLSelector::Objects()
{
    return mSelectedObjs;
}

void LP_GLSelector::DrawLabel(QOpenGLContext *ctx, QSurface *surf, QOpenGLFramebufferObject *fbo, const LP_RendererCam &cam)
{
    Q_UNUSED(surf)

    auto f = ctx->extraFunctions();
    f->glEnable( GL_DEPTH_TEST );

    f->glCullFace(GL_BACK);
    f->glEnable(GL_CULL_FACE);
    f->glEnable(GL_BLEND);
    f->glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    f->glDepthFunc(GL_LEQUAL);

    fbo->bind();
    auto proj = cam->ProjectionMatrix(),
         view = cam->ViewMatrix();

    mLock.lockForRead();

    mProgram->bind();

    mProgram->setUniformValue("m4_mvp", proj*view);
    mProgram->setUniformValue("v4_color", QVector4D(1.0f,0.1f,0.1f,0.5f));

    for ( auto &o : qAsConst(mSelectedObjs) ){
        o.lock()->DrawSelect(ctx, surf, fbo, mProgram, cam);
    }

    mProgram->release();

    mLock.unlock();

    fbo->release();
    f->glDisable( GL_BLEND );
    f->glDisable( GL_DEPTH_TEST );
    f->glDisable( GL_CULL_FACE );
    f->glDepthFunc(GL_LESS);
}

void LP_GLSelector::InitialGL(QOpenGLContext *ctx, QSurface *surf)
{
    std::string vsh =
            "attribute vec3 a_pos;\n"
            "uniform mat4 m4_mvp;\n"
            "void main(){\n"
            "   gl_Position = m4_mvp*vec4(a_pos, 1.0);\n"
            "}";
    std::string fsh =
            "uniform vec4 v4_color;\n"
            "void main(){\n"
            "   gl_FragColor = v4_color;\n"
            "}";

    ctx->makeCurrent(surf);

    auto prog = new QOpenGLShaderProgram;
    prog->addShaderFromSourceCode(QOpenGLShader::Vertex,vsh.c_str());
    prog->addShaderFromSourceCode(QOpenGLShader::Fragment,fsh.data());
    if ( !prog->create() || !prog->link()){
        delete prog;
        ctx->doneCurrent();
        throw std::runtime_error("Initialize selection program failed");
    }
    mProgram = prog;

    ctx->doneCurrent();
}

void LP_GLSelector::DestroyGL(QOpenGLContext *ctx, QSurface *surf)
{
    ctx->makeCurrent(surf);

    delete mProgram;
    mProgram = nullptr;

    ctx->doneCurrent();
}

void LP_GLSelector::Clip(float &sx, float &sy, int &w, int &h, int maxx, int maxy, int minx, int miny)
{
    if ( sx < minx ){
        w += sx;
        sx = minx;
    }
    if ( sy < miny ){
        h += sy;
        sy = miny;
    }
    if ( sx + w >= maxx ){
        w = maxx - 1 - sx;
    }
    if ( sy + h >= maxy ){
        h = maxy - 1 - sy;
    }
}

std::vector<LP_Objectw> LP_GLSelector::SelectInWorld(const QString &renderername, const QPoint &pos, int w, int h)
{
    auto v_ = g_Renderers.find(renderername);
    if ( g_Renderers.end() == v_ ){
        qDebug() << "Unknown renderer : " << renderername;
        return {};
    }

    auto pDoc = &LP_Document::gDoc;

    std::vector<uint> selections;
    LP_GLRendererCB cb = [pDoc, center=pos, xspan=w, yspan=h, &selections]
            (QOpenGLContext *ctx, QSurface *surf, QOpenGLFramebufferObject *_fbo, const LP_RendererCam &cam, void*){
        Q_UNUSED(_fbo)
        auto w = cam->ResolutionX(),
             h = cam->ResolutionY();

        float org[2] = {center.x() - 0.5f*xspan, h - center.y() - 0.5f * yspan};
        int sw(xspan), sh(yspan);

        LP_GLSelector::Clip(org[0], org[1], sw, sh, w, h);

        if ( 0 >= sw || 0 >= sh ){
            return;
        }
        auto f = ctx->extraFunctions();
        ctx->makeCurrent(surf);

        auto fbo = new QOpenGLFramebufferObject(w, h,
                                            QOpenGLFramebufferObject::Depth);

        static std::string vsh =
                "attribute vec3 a_pos;\n"
                "uniform mat4 m4_mvp;\n"
                "void main(){\n"
                "   gl_Position = m4_mvp*vec4(a_pos, 1.0);\n"
                "}";
        static std::string fsh =
                "uniform vec4 v4_color;\n"
                "void main(){\n"
                "   gl_FragColor = v4_color;\n"
                "}";



        auto prog = new QOpenGLShaderProgram;
        prog->addShaderFromSourceCode(QOpenGLShader::Vertex,vsh.c_str());
        prog->addShaderFromSourceCode(QOpenGLShader::Fragment,fsh.data());
        prog->bind();
        prog->setUniformValue("m4_mvp", cam->ProjectionMatrix() * cam->ViewMatrix());

        f->glViewport(0,0, w, h);
        f->glEnable(GL_SCISSOR_TEST);
        f->glScissor(org[0],org[1],sw, sh);

        fbo->bind();

        f->glClearColor( 0.0, 0.0, 0.0, 0.0 );
        f->glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
        f->glEnable( GL_DEPTH_TEST );

        auto list = pDoc->Objects();
        Q_ASSERT(list.size() < std::numeric_limits<int>::max());
        const constexpr uint _start = 1;
        uint i=_start;
        constexpr float denom = 1.0f/255.0f;
        for ( auto &o : list ){
            uchar r = (i & 0x000000FF) >>  0;
            uchar g = (i & 0x0000FF00) >>  8;
            uchar b = (i & 0x00FF0000) >> 16;
            uchar a = (i & 0xFF000000) >> 24;

            prog->setUniformValue("v4_color",r*denom,g*denom,b*denom,a*denom);
            o.lock()->DrawSelect(ctx, surf, fbo, prog, cam);
            ++i;
        }

        f->glFlush();
        f->glFinish();

        f->glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

        const uint nPix  = sw*sh;
        constexpr uint nChn  = 4;
        unsigned char *color = new unsigned char[nPix*nChn];
        float *depth    = new float[nPix];

        f->glReadPixels( org[0], org[1],
                         sw, sh,
                         GL_RGBA, GL_UNSIGNED_BYTE, color);

        f->glReadPixels(org[0], org[1],
                        sw, sh,
                        GL_DEPTH_COMPONENT, GL_FLOAT , depth);

        fbo->release();
        prog->release();

        f->glDisable(GL_SCISSOR_TEST);
        ctx->doneCurrent();

        QMultiMap<float, uint> depthMap;
        QMap<uint, QVector4D> colorMap;
        unsigned char* c(nullptr);
        constexpr uint bb   = 256*256,
                       aa   = bb*256;
        // Convert the color back to an integer ID
        for ( int i=0; i<sw; ++i ){
            for( int j=0; j<sh; ++j ){
                const int index = i*sh + j;
                c   = &color[index*nChn];
                if ( c[0] > 0 ){
                    uint pickedID =
                        c[0] +
                        c[1] * 256 +
                        c[2] * bb +
                        c[3] * aa;

                    pickedID -= _start;

                    auto selIt  = colorMap.find(pickedID);
                    if (colorMap.end() == selIt){
                        depthMap.insert( depth[index], pickedID );
                        colorMap.insert(pickedID,
                                         QVector4D(c[0], c[1], c[2], c[3]));
                    }
                }
            }
        }

        delete []depth;
        delete []color;
        delete fbo;
        delete prog;

        for ( auto it = depthMap.begin(); it != depthMap.end(); ++it ){
            selections.emplace_back(it.value());
        }
    };
    auto renderer = v_.value();
    QMetaObject::invokeMethod(renderer,
                              "RunInContext",
                              Qt::BlockingQueuedConnection,
                              Q_ARG(LP_GLRendererCB,cb));

    if ( selections.empty()){
        return {};
    }

    std::vector<LP_Objectw> l;
    const size_t nO = pDoc->Objects().size();
    auto it = pDoc->Objects().begin();
    for ( auto &i : selections ){
        assert(i<nO);
        l.emplace_back(pDoc->FindObject((*std::next(it,i)).lock()->Uuid()));
    }
    assert(!l.empty());
    //qDebug() << selections << l;
    return l;
}

std::vector<LP_Objectw> LP_GLSelector::ProxySelectInWorld(const QString &renderername, const QPoint &pos, int w, int h)
{
    auto v_ = g_Renderers.find(renderername);
    if ( g_Renderers.end() == v_ ){
        qDebug() << "Unknown renderer : " << renderername;
        return {};
    }

    auto pDoc = &LP_Document::gDoc;

    std::vector<uint> selections;
    LP_GLRendererCB cb = [pDoc, center=pos, xspan=w, yspan=h, &selections]
            (QOpenGLContext *ctx, QSurface *surf, QOpenGLFramebufferObject *_fbo, const LP_RendererCam &cam, void*){
        Q_UNUSED(_fbo)
        auto w = cam->ResolutionX(),
             h = cam->ResolutionY();

        float org[2] = {center.x() - 0.5f*xspan, h - center.y() - 0.5f * yspan};
        int sw(xspan), sh(yspan);

        LP_GLSelector::Clip(org[0], org[1], sw, sh, w, h);

        if ( 0 >= sw || 0 >= sh ){
            return;
        }
        auto f = ctx->extraFunctions();
        ctx->makeCurrent(surf);

        auto fbo = new QOpenGLFramebufferObject(w, h,
                                            QOpenGLFramebufferObject::Depth);

        static std::string vsh =
                "attribute vec3 a_pos;\n"
                "uniform mat4 m4_mvp;\n"
                "void main(){\n"
                "   gl_Position = m4_mvp*vec4(a_pos, 1.0);\n"
                "}";
        static std::string fsh =
                "uniform vec4 v4_color;\n"
                "void main(){\n"
                "   gl_FragColor = v4_color;\n"
                "}";



        auto prog = new QOpenGLShaderProgram;
        prog->addShaderFromSourceCode(QOpenGLShader::Vertex,vsh.c_str());
        prog->addShaderFromSourceCode(QOpenGLShader::Fragment,fsh.data());
        prog->bind();
        prog->setUniformValue("m4_mvp", cam->ProjectionMatrix() * cam->ViewMatrix());

        f->glViewport(0,0, w, h);
        f->glEnable(GL_SCISSOR_TEST);
        f->glScissor(org[0],org[1],sw, sh);

        fbo->bind();

        f->glClearColor( 0.0, 0.0, 0.0, 0.0 );
        f->glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
        f->glEnable( GL_DEPTH_TEST );

        auto list = pDoc->Objects();
        Q_ASSERT(list.size() < std::numeric_limits<int>::max());
        const constexpr uint _start = 1;
        uint i=_start;
        constexpr float denom = 1.0f/255.0f;
        for ( auto &o : list ){
            uchar r = (i & 0x000000FF) >>  0;
            uchar g = (i & 0x0000FF00) >>  8;
            uchar b = (i & 0x00FF0000) >> 16;
            uchar a = (i & 0xFF000000) >> 24;

            prog->setUniformValue("v4_color",r*denom,g*denom,b*denom,a*denom);
            o.lock()->DrawSelect(ctx, surf, fbo, prog, cam);
            ++i;
        }

        f->glFlush();
        f->glFinish();

        f->glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

        const uint nPix  = sw*sh;
        constexpr uint nChn  = 4;
        unsigned char *color = new unsigned char[nPix*nChn];
        float *depth    = new float[nPix];

        f->glReadPixels( org[0], org[1],
                         sw, sh,
                         GL_RGBA, GL_UNSIGNED_BYTE, color);

        f->glReadPixels(org[0], org[1],
                        sw, sh,
                        GL_DEPTH_COMPONENT, GL_FLOAT , depth);

        fbo->release();
        prog->release();

        f->glDisable(GL_SCISSOR_TEST);
        ctx->doneCurrent();

        QMultiMap<float, uint> depthMap;
        QMap<uint, QVector4D> colorMap;
        unsigned char* c(nullptr);
        constexpr uint bb   = 256*256,
                       aa   = bb*256;
        // Convert the color back to an integer ID
        for ( int i=0; i<sw; ++i ){
            for( int j=0; j<sh; ++j ){
                const int index = i*sh + j;
                c   = &color[index*nChn];
                if ( c[0] > 0 ){
                    uint pickedID =
                        c[0] +
                        c[1] * 256 +
                        c[2] * bb +
                        c[3] * aa;

                    pickedID -= _start;

                    auto selIt  = colorMap.find(pickedID);
                    if (colorMap.end() == selIt){
                        depthMap.insert( depth[index], pickedID );
                        colorMap.insert(pickedID,
                                         QVector4D(c[0], c[1], c[2], c[3]));
                    }
                }
            }
        }

        delete []depth;
        delete []color;
        delete fbo;
        delete prog;

        for ( auto it = depthMap.begin(); it != depthMap.cend(); ++it ){
            selections.emplace_back(it.value());
        }
    };
    auto renderer = v_.value();
    QMetaObject::invokeMethod(renderer,
                              "RunInContext",
                              Qt::BlockingQueuedConnection,
                              Q_ARG(LP_GLRendererCB,cb));

    if ( selections.empty()){
        return {};
    }

    std::vector<LP_Objectw> l;
    const size_t nO = pDoc->Objects().size();
    auto it = pDoc->Objects().begin();
    for ( auto &i : selections ){
        assert(i<nO);
        l.emplace_back(pDoc->FindObject((*std::next(it,i)).lock()->Uuid()));
    }
    assert(!l.empty());
    //qDebug() << selections << l;
    return l;
}

void LP_GLSelector::SelectCustom(const QString &renderername, void *cb, void *data)
{
    auto v_ = g_Renderers.find(renderername);
    if ( g_Renderers.cend() == v_ ){
        return;
    }

    auto renderer = v_.value();

    auto _cb = (LP_GLRendererCB*)cb;
    QMetaObject::invokeMethod(renderer,
                              "RunInContext",
                              Qt::BlockingQueuedConnection,
                              Q_ARG(LP_GLRendererCB,*_cb),
                              Q_ARG(void*, data));

}

std::vector<uint> LP_GLSelector::SelectPoints3D(const QString &renderername, const std::vector<float[3]> &pts, const QPoint &pos, bool mask, int w, int h)
{
    auto v_ = g_Renderers.find(renderername);
    if ( g_Renderers.end() == v_ ){
        return std::vector<uint>();
    }

    std::vector<uint> selections;
    LP_GLRendererCB cb = [center=pos, xspan=w, yspan=h, &mask, &pts, &selections]
            (QOpenGLContext *ctx, QSurface *surf, QOpenGLFramebufferObject *_fbo, const LP_RendererCam &cam, void*){

        auto w = cam->ResolutionX(),
             h = cam->ResolutionY();

        int   sw(xspan),
              sh(yspan);
        float org[2] = {center.x() - 0.5f*sw, h - center.y() - 0.5f * sh};

        LP_GLSelector::Clip(org[0], org[1], sw, sh, w, h);

        if ( 0 >= sw || 0 >= sh ){
            return;
        }

        auto f = ctx->extraFunctions();
        ctx->makeCurrent(surf);

        auto fbo = new QOpenGLFramebufferObject(w, h,
                                            QOpenGLFramebufferObject::Depth);

        constexpr char vsh[] =
                "attribute vec3 a_pos;\n"
                "attribute vec3 a_color;\n"
                "uniform float f_pointSize;\n"
                "uniform mat4 m4_mvp;\n"
                "varying vec3 color;\n"
                "void main(){\n"
                "   gl_Position = m4_mvp*vec4(a_pos, 1.0);\n"
                "   gl_PointSize = f_pointSize;\n"
                "   color = a_color;\n"
                "}";
        constexpr char fsh[] =
                "varying vec3 color;\n"
                "void main(){\n"
                "   gl_FragColor = vec4(color, 1.0);\n"
                "}";



        constexpr uint _start = 0;    //This must be smaller than 2^24 - pos.size()

        const std::vector<QVector3D> &_color = LP_GLSelector::g_24ColorVector;
        auto prog = new QOpenGLShaderProgram;
        prog->addShaderFromSourceCode(QOpenGLShader::Vertex,vsh);
        prog->addShaderFromSourceCode(QOpenGLShader::Fragment,fsh);
        prog->bind();
        prog->setUniformValue("m4_mvp", cam->ProjectionMatrix() * cam->ViewMatrix());
        prog->setUniformValue("f_pointSize", std::max<float>(xspan, yspan));
        prog->enableAttributeArray("a_color");
        prog->setAttributeArray("a_color", _color.data());

        f->glViewport(0,0, w, h);
        f->glEnable(GL_SCISSOR_TEST);
        f->glScissor(org[0],org[1],sw, sh);

        f->glEnable( GL_DEPTH_TEST );
        f->glEnable( GL_PROGRAM_POINT_SIZE );

        fbo->bind();

        f->glClearColor( 0.0f, 0.0f, 0.0f, 0.0f );
        f->glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
        if ( mask ){//Copy the depth buffer
            QOpenGLFramebufferObject::blitFramebuffer(fbo, _fbo, GL_DEPTH_BUFFER_BIT);
        }

        prog->enableAttributeArray("a_pos");
        prog->setAttributeArray("a_pos", pts.front(), 3);

        f->glDrawArrays(GL_POINTS, _start, GLsizei(pts.size()));


        f->glFlush();
        f->glFinish();

        f->glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

        const uint nPix  = sw*sh;
        constexpr int nChn  = 4;
        unsigned char *color = new unsigned char[nPix*nChn];
        float *depth    = new float[nPix];

        f->glReadPixels( org[0], org[1],
                         sw, sh,
                         GL_RGBA, GL_UNSIGNED_BYTE, color);

        f->glReadPixels(org[0], org[1],
                        sw, sh,
                        GL_DEPTH_COMPONENT, GL_FLOAT , depth);

        prog->disableAttributeArray("a_pos");
        prog->disableAttributeArray("a_color");

        fbo->release();
        prog->release();

        f->glDisable(GL_SCISSOR_TEST);
        f->glDisable( GL_DEPTH_TEST );
        f->glDisable( GL_PROGRAM_POINT_SIZE );
        ctx->doneCurrent();

        QMultiMap<float, uint> depthMap;
        QMap<uint, QVector4D> colorMap;
        unsigned char* c(nullptr);
        constexpr uint  bb   = 256*256;
        // Convert the color back to an integer ID
        for ( int i=0; i<sw; ++i ){
            for( int j=0; j<sh; ++j ){
                const uint index = i*sh + j;
                c   = &color[index*nChn];
                if ( c[3] == 255 ){
                    uint pickedID = c[0] + c[1] * 256 + c[2] * bb;

                    pickedID -= _start;

                    auto selIt  = colorMap.find(pickedID);
                    if (colorMap.end() == selIt){
                        depthMap.insert( depth[index], pickedID );
                        colorMap.insert(pickedID,
                                         QVector4D(c[0], c[1], c[2], c[3]));
                    }
                }
            }
        }

        delete []depth;
        delete []color;
        delete fbo;
        delete prog;


        for ( auto it = depthMap.begin(); it != depthMap.cend(); ++it ){
            selections.emplace_back(it.value());
        }
    };

    auto renderer = v_.value();

    QMetaObject::invokeMethod(renderer,
                              "RunInContext",
                              Qt::BlockingQueuedConnection,
                              Q_ARG(LP_GLRendererCB,cb));

    return selections;
}

std::vector<uint> LP_GLSelector::SelectPoints3D(const QString &renderername, const float *pts, const size_t &npts, const QPoint &pos, bool mask, int w, int h)
{
    auto v_ = g_Renderers.find(renderername);
    if ( g_Renderers.cend() == v_ ){
        return std::vector<uint>();
    }

    std::vector<uint> selections;
    LP_GLRendererCB cb = [center=pos, xspan=w, yspan=h, &mask, &pts, &npts, &selections]
            (QOpenGLContext *ctx, QSurface *surf, QOpenGLFramebufferObject *_fbo, const LP_RendererCam &cam, void*){

        auto w = cam->ResolutionX(),
             h = cam->ResolutionY();

        int   sw(xspan),
              sh(yspan);
        float org[2] = {center.x() - 0.5f*sw, h - center.y() - 0.5f * sh};

        LP_GLSelector::Clip(org[0], org[1], sw, sh, w, h);

        if ( 0 >= sw || 0 >= sh ){
            return;
        }

        auto f = ctx->extraFunctions();
        ctx->makeCurrent(surf);

        auto fbo = new QOpenGLFramebufferObject(w, h,
                                            QOpenGLFramebufferObject::Depth);

        constexpr char vsh[] =
                "attribute vec3 a_pos;\n"
                "attribute vec3 a_color;\n"
                "uniform float f_pointSize;\n"
                "uniform mat4 m4_mvp;\n"
                "varying vec3 color;\n"
                "void main(){\n"
                "   gl_Position = m4_mvp*vec4(a_pos, 1.0);\n"
                "   gl_PointSize = f_pointSize;\n"
                "   color = a_color;\n"
                "}";
        constexpr char fsh[] =
                "varying vec3 color;\n"
                "void main(){\n"
                "   gl_FragColor = vec4(color, 1.0);\n"
                "}";

        constexpr uint _start = 0;    //This must be smaller than 2^24 - pos.size()

        const std::vector<QVector3D> &_color = LP_GLSelector::g_24ColorVector;
        auto prog = new QOpenGLShaderProgram;
        prog->addShaderFromSourceCode(QOpenGLShader::Vertex,vsh);
        prog->addShaderFromSourceCode(QOpenGLShader::Fragment,fsh);
        prog->bind();
        prog->setUniformValue("m4_mvp", cam->ProjectionMatrix() * cam->ViewMatrix());
        prog->setUniformValue("f_pointSize", 1.f/*std::max<float>(xspan, yspan)*/);
        prog->enableAttributeArray("a_color");
        prog->setAttributeArray("a_color", _color.data());

        f->glViewport(0,0, w, h);
        f->glEnable(GL_SCISSOR_TEST);
        f->glScissor(org[0],org[1],sw, sh);

        f->glEnable( GL_DEPTH_TEST );
        f->glDepthFunc( GL_LEQUAL );
        f->glEnable( GL_PROGRAM_POINT_SIZE );

        fbo->bind();

        f->glClearColor( 0.0f, 0.0f, 0.0f, 0.0f );
        f->glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
        if ( mask ){//Copy the depth buffer
            QOpenGLFramebufferObject::blitFramebuffer(fbo, _fbo, GL_DEPTH_BUFFER_BIT);
        }

        prog->enableAttributeArray("a_pos");
        prog->setAttributeArray("a_pos", pts, 3);

        f->glDrawArrays(GL_POINTS, _start, GLsizei(npts));


        f->glFlush();
        f->glFinish();

        f->glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

        const uint nPix  = sw*sh;
        constexpr int nChn  = 4;
        unsigned char *color = new unsigned char[nPix*nChn];
        float *depth    = new float[nPix];

        f->glReadPixels( org[0], org[1],
                         sw, sh,
                         GL_RGBA, GL_UNSIGNED_BYTE, color);

        f->glReadPixels(org[0], org[1],
                        sw, sh,
                        GL_DEPTH_COMPONENT, GL_FLOAT , depth);

        prog->disableAttributeArray("a_pos");
        prog->disableAttributeArray("a_color");

        fbo->release();
        prog->release();

        f->glDisable(GL_SCISSOR_TEST);
        f->glDisable( GL_DEPTH_TEST );
        f->glDisable( GL_PROGRAM_POINT_SIZE );
        ctx->doneCurrent();

        QMultiMap<float, uint> depthMap;
        QMap<uint, QVector4D> colorMap;
        unsigned char* c(nullptr);
        constexpr uint  bb   = 256*256;
        // Convert the color back to an integer ID
        for ( int i=0; i<sw; ++i ){
            for( int j=0; j<sh; ++j ){
                const uint index = i*sh + j;
                c   = &color[index*nChn];
                if ( c[3] == 255 ){
                    uint pickedID = c[0] + c[1] * 256 + c[2] * bb;

                    pickedID -= _start;

                    auto selIt  = colorMap.find(pickedID);
                    if (colorMap.end() == selIt){
                        depthMap.insert( depth[index], pickedID );
                        colorMap.insert(pickedID,
                                         QVector4D(c[0], c[1], c[2], c[3]));
                    }
                }
            }
        }

        delete []depth;
        delete []color;
        delete fbo;
        delete prog;


        for ( auto it = depthMap.begin(); it != depthMap.cend(); ++it ){
            selections.emplace_back(it.value());
        }
    };

    auto renderer = v_.value();
    QMetaObject::invokeMethod(renderer,
                              "RunInContext",
                              Qt::BlockingQueuedConnection,
                              Q_ARG(LP_GLRendererCB,cb));

    return selections;
}

void LP_GLSelector::SetRubberBand(QRubberBand *rb)
{
    mRubberBand = rb;
}

//LP_GLRenderer *LP_GLSelector::GetRenderer(const QString &name) const
//{
//    return mRenderers.find(name).value();
//}

//void LP_GLSelector::AddRenderer(LP_GLRenderer *renderer)
//{
//    mRenderers[renderer->objectName()] = renderer;
//}

QRubberBand *LP_GLSelector::RubberBand() const
{
    return mRubberBand;
}
