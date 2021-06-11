#include "lp_pointcloud.h"
#include "lp_renderercam.h"

#include <QOpenGLFramebufferObject>
#include <QOpenGLContext>
#include <QOpenGLShaderProgram>
#include <QOpenGLExtraFunctions>

LP_PointCloudImpl::~LP_PointCloudImpl()
{
    Q_ASSERT(mPrograms.empty());
    Q_ASSERT(!mVBO);
    Q_ASSERT(!mIndices);
}

void LP_PointCloudImpl::SetPoints(std::vector<QVector3D> &&pc)
{
    QWriteLocker loocker(&mLock);
    mPoints = pc;
}

const std::vector<QVector3D> &LP_PointCloudImpl::Points() const
{
    QReadLocker locker(&mLock);
    return mPoints;
}

void LP_PointCloudImpl::SetNormals(std::vector<QVector3D> &&pc)
{
    QWriteLocker loocker(&mLock);
    mNormals = pc;
}

const std::vector<QVector3D> &LP_PointCloudImpl::Normals() const
{
    QReadLocker locker(&mLock);
    return mPoints;
}

LP_PointCloudImpl::LP_PointCloudImpl(LP_Object parent) : LP_GeometryImpl(parent)
  ,mVBO(nullptr)
  ,mIndices(nullptr)
{

}

bool LP_PointCloudImpl::DrawSetup(QOpenGLContext *ctx, QSurface *surf, QVariant &option)
{
    QWriteLocker locker(&mLock);
    ctx->makeCurrent(surf);

    std::string vsh, fsh;
    if ( option.toString() == "Normal"){
        vsh =
                "attribute vec3 a_pos;\n"
                "attribute vec3 a_norm;\n"
                "uniform mat4 m4_mvp;\n"
                "varying vec3 normal;\n"
                "void main(){\n"
                "   normal = a_norm;\n"
                "   gl_Position = m4_mvp*vec4(a_pos, 1.0);\n"
                "}";
        fsh =
                "varying vec3 normal;\n"
                "void main(){\n"
                "   vec3 n = 0.5 * ( vec3(1.0) + normalize(normal));\n"
                "   gl_FragColor = vec4(n,1.0);\n"
                "}";
    }else{
        vsh =
                "attribute vec3 a_pos;\n"
                "attribute vec3 a_norm;\n"
                "uniform mat4 m4_view;\n"
                "uniform mat4 m4_mvp;\n"
                "uniform mat3 m3_normal;\n"
                "varying vec3 normal;\n"
                "varying vec3 pos;\n"
                "void main(){\n"
                "   pos = vec3( m4_view * vec4(a_pos, 1.0));\n"
                "   normal = m3_normal * a_norm;\n"
                "   gl_Position = m4_mvp*vec4(a_pos, 1.0);\n"
                "}";
        fsh =
                "varying vec3 normal;\n"
                "varying vec3 pos;\n"
                "uniform vec3 v3_color;\n"
                "void main(){\n"
                "   vec3 lightPos = vec3(0.0, 1000.0, 0.0);\n"
                "   vec3 viewDir = normalize( - pos);\n"
                "   vec3 lightDir = normalize(lightPos - pos);\n"
                "   vec3 H = normalize(viewDir + lightDir);\n"
                "   vec3 N = normalize(normal);\n"
                "   vec3 ambi = v3_color;\n"
                "   float Kd = max(dot(H, N), 0.0);\n"
                "   vec3 diff = Kd * vec3(0.2, 0.2, 0.2);\n"
                "   vec3 color = ambi + diff;\n"
                "   float Ks = pow( Kd, 80.0 );\n"
                "   vec3 spec = Ks * vec3(0.5, 0.5, 0.5);\n"
                "   color += spec;\n"
                "   gl_FragColor = vec4(color,1.0);\n"
                "}";
    }

    if ( !mPrograms.contains(option.toString())){
        auto prog = new QOpenGLShaderProgram;
        prog->addShaderFromSourceCode(QOpenGLShader::Vertex,vsh.c_str());
        prog->addShaderFromSourceCode(QOpenGLShader::Fragment,fsh.data());
        if ( !prog->create() || !prog->link()){
            qCritical() << prog->log();
        }
        mPrograms[option.toString()] = prog;
    }


    if ( mGLInitialized ){
        return true;
    }

    mVBO = new QOpenGLBuffer(QOpenGLBuffer::VertexBuffer);
    mVBO->setUsagePattern(QOpenGLBuffer::StreamDraw);
    mVBO->create();

    auto nVs = mPoints.size();
    auto nNs = mNormals.size();
    const int _a = int( nVs * sizeof(QVector3D)+
                        nNs * sizeof(QVector3D));

    mVBO->bind();
    mVBO->allocate( _a );
    //mVBO->allocate( m->points(), int( m->n_vertices() * sizeof(*m->points())));
    auto ptr = static_cast<float*>(mVBO->mapRange(0, _a,
                                                          QOpenGLBuffer::RangeWrite |
                                                          QOpenGLBuffer::RangeInvalidateBuffer));
    auto pptr = mPoints.data();
    auto nptr = mNormals.empty() ? nullptr : mNormals.data();

    for ( size_t i=0; i<nVs; ++i, ++pptr){
        memcpy(ptr, pptr, sizeof(QVector3D));
        ptr += sizeof(QVector3D)/sizeof(*ptr);

        if ( nptr ){
            memcpy(ptr, nptr, sizeof(QVector3D));
            ptr += sizeof(QVector3D)/sizeof(*ptr);
            ++nptr;
        }

        if ( mBBmin.x() > (*pptr)[0]){
            mBBmin.setX((*pptr)[0]);
        }else if ( mBBmax.x() < (*pptr)[0]){
            mBBmax.setX((*pptr)[0]);
        }

        if ( mBBmin.y() > (*pptr)[1]){
            mBBmin.setY((*pptr)[1]);
        }else if ( mBBmax.y() < (*pptr)[1]){
            mBBmax.setY((*pptr)[1]);
        }

        if ( mBBmin.z() > (*pptr)[2]){
            mBBmin.setZ((*pptr)[2]);
        }else if ( mBBmax.z() < (*pptr)[2]){
            mBBmax.setZ((*pptr)[2]);
        }

        //qDebug() << QString("%1, %2, %3").arg((*nptr)[0]).arg((*nptr)[1]).arg((*nptr)[2]);
    }
    mVBO->unmap();
    mVBO->release();


    //===============================================================
    //Indice buffer
    //TODO: Not save for non-triangular mesh
    std::vector<uint> indices(nVs);
    auto i_it = indices.begin();
    for ( size_t i=0; i<nVs; ++i){
        (*i_it++) = i;
   }

    mIndices = new QOpenGLBuffer(QOpenGLBuffer::IndexBuffer);
    mIndices->setUsagePattern(QOpenGLBuffer::StaticDraw);

    const int a_ = int( indices.size()) * sizeof(indices[0]);
    mIndices->create();
    mIndices->bind();
    mIndices->allocate( a_ );
    auto iptr = static_cast<uint*>(mIndices->mapRange(0, a_,
                                                      QOpenGLBuffer::RangeWrite |
                                                      QOpenGLBuffer::RangeInvalidateBuffer));
    memcpy(iptr, indices.data(), indices.size() * sizeof(indices[0]));
    mIndices->unmap();
    mIndices->release();

    ctx->doneCurrent();

    mGLInitialized = true;
    return true;
}

void LP_PointCloudImpl::Draw(QOpenGLContext *ctx, QSurface *surf, QOpenGLFramebufferObject *fbo, const LP_RendererCam &cam, QVariant &option)
{
    Q_UNUSED(surf)


    auto model = ModelTrans();
    auto proj = cam->ProjectionMatrix();
    auto view = cam->ViewMatrix() * model;
    auto f = ctx->extraFunctions();

    auto it = mPrograms.find(option.toString());
    if ( it == mPrograms.cend()){
        return;
    }
    auto prog = it.value();

    fbo->bind();

    prog->bind();
    prog->setUniformValue("m4_mvp", proj*view );
    prog->setUniformValue("m4_view", view );
    prog->setUniformValue("m3_normal", view.normalMatrix());
    prog->setUniformValue("v3_color", QVector3D(0.8,0.8,0.8));

    prog->enableAttributeArray("a_pos");

    mVBO->bind();
    mIndices->bind();
    if (mNormals.empty()){
        prog->setAttributeBuffer("a_pos",GL_FLOAT, 0, 3, 12);
    }else{
        prog->enableAttributeArray("a_norm");
        prog->setAttributeBuffer("a_pos",GL_FLOAT, 0, 3, 24);
        prog->setAttributeBuffer("a_norm",GL_FLOAT, 12, 3, 24);
    }

    f->glDrawElements(GL_POINTS, GLsizei(mPoints.size()), GL_UNSIGNED_INT, nullptr);

    mVBO->release();
    mIndices->release();

    prog->disableAttributeArray("a_pos");
    prog->disableAttributeArray("a_norm");
    prog->release();

    fbo->release();
}

bool LP_PointCloudImpl::DrawCleanUp(QOpenGLContext *ctx, QSurface *surf)
{
    QWriteLocker locker(&mLock);
    ctx->makeCurrent(surf);

    if ( mVBO ){
        mVBO->release();
        delete mVBO;
        mVBO = nullptr;
    }

    if ( mIndices ){
        mIndices->release();
        delete mIndices;
        mIndices = nullptr;
    }

    for ( auto p : qAsConst(mPrograms) ){
        p->release();
        delete p;
    }
    mPrograms.clear();
    ctx->doneCurrent();
    mGLInitialized = false; //Unitialized

    return true;
}

void LP_PointCloudImpl::DrawSelect(QOpenGLContext *ctx, QSurface *surf, QOpenGLFramebufferObject *fbo, QOpenGLShaderProgram *prog, const LP_RendererCam &cam)
{
    Q_UNUSED(surf)
    Q_UNUSED(fbo)
    Q_UNUSED(cam)

    auto f = ctx->extraFunctions();

    mVBO->bind();
    mIndices->bind();
    prog->setUniformValue("m4_mvp", cam->ProjectionMatrix() * cam->ViewMatrix() * mTrans);
    prog->enableAttributeArray("a_pos");
    prog->setAttributeBuffer("a_pos",GL_FLOAT, 0, 3, 12);

    f->glDrawElements(GL_POINTS, GLsizei(mPoints.size()), GL_UNSIGNED_INT, nullptr);

    prog->disableAttributeArray("a_pos");
    mVBO->release();
    mIndices->release();
}

void LP_PointCloudImpl::_Dump(QDebug &debug)
{
    LP_GeometryImpl::_Dump(debug);
    debug.nospace() << "#V : " << mPoints.size() << ", "
                    << "#N : " << mNormals.size()
                    << "\n";
}

