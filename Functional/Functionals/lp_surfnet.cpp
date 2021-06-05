#include "lp_surfnet.h"
#include "extern/Mesh.h"
#include "extern/LBO.h"
#include "lp_renderercam.h"
#include "renderer/lp_glselector.h"

#include <QPushButton>
#include <QFileDialog>
#include <QVBoxLayout>
#include <QtConcurrent/QtConcurrent>
#include <QOpenGLContext>
#include <QOpenGLShaderProgram>
#include <QOpenGLExtraFunctions>
#include <QOpenGLFramebufferObject>
#include <QMouseEvent>
#include <QPainter>

REGISTER_FUNCTIONAL_IMPLEMENT(LP_SURFNet, SURFNet, menuTool)

LP_SURFNet::LP_SURFNet(QObject *parent) : LP_Functional(parent)
{

}

QWidget *LP_SURFNet::DockUi()
{
    mWidget = std::make_shared<QWidget>();
    auto widget = mWidget.get();

    QVBoxLayout *layout = new QVBoxLayout;
    QPushButton *button = new QPushButton("Import Mesh");
    layout->addWidget(button);

    layout->addStretch();
    widget->setLayout(layout);

    connect(button, &QPushButton::clicked, [this](){
        auto fileX = QFileDialog::getOpenFileName(0,"Select a mesh X");
        if ( fileX.isEmpty()) {
            return;
        }
        auto fileY = QFileDialog::getOpenFileName(0,"Select a mesh Y");
        if ( fileY.isEmpty()) {
            return;
        }
        auto corr = QFileDialog::getExistingDirectory(0,"Select the correspondence folder");
        if ( corr.isEmpty()) {
            return;
        }
        mCorrPath = corr;
        auto future = QtConcurrent::run(&mPool,[fileX,fileY,corr,this](){
            try {
                mMeshX = std::make_shared<CMesh>();
                if (!mMeshX->Load(fileX.toUtf8())){
                    qDebug() << "Load failed!";
                    mMeshX.reset();
                    return;
                }
                mVx.resize(mMeshX->m_nVertex);
                mNx.resize(mMeshX->m_nVertex);
                for ( uint i=0; i<mMeshX->m_nVertex; ++i ){
                    auto *p = &mMeshX->m_pVertex[i].m_vPosition;
                    mVx[i].setX(p->x);
                    mVx[i].setY(p->y);
                    mVx[i].setZ(p->z);
                    p = &mMeshX->m_pVertex[i].m_vNormal;
                    mNx[i].setX(p->x);
                    mNx[i].setY(p->y);
                    mNx[i].setZ(p->z);
                }

                mIx.clear();
                for ( uint i=0; i<mMeshX->m_nFace; ++i ){
                    auto pF = mMeshX->m_pFace[i];
                    for ( auto j=0; j<pF.m_nType; ++j ){
                        mIx.emplace_back(pF.m_piVertex[j]);
                    }
                }
                qDebug() << "Indix size : " << mIx.size() << " Vertex: " << mVx.size()
                         << " face : " << mMeshX->m_nFace;

                mMeshY = std::make_shared<CMesh>();
                if (!mMeshY->Load(fileY.toUtf8())){
                    mMeshY.reset();
                    qDebug() << "Load failed!";
                    return;
                }
                mVy.resize(mMeshY->m_nVertex);
                mNy.resize(mMeshY->m_nVertex);
                for ( uint i=0; i<mMeshY->m_nVertex; ++i ){
                    auto *p = &mMeshY->m_pVertex[i].m_vPosition;
                    mVy[i].setX(p->x);
                    mVy[i].setY(p->y);
                    mVy[i].setZ(p->z);
                    p = &mMeshY->m_pVertex[i].m_vNormal;
                    mNy[i].setX(p->x);
                    mNy[i].setY(p->y);
                    mNy[i].setZ(p->z);
                }

                mIy.clear();
                for ( uint i=0; i<mMeshY->m_nFace; ++i ){
                    auto pF = mMeshY->m_pFace[i];
                    for ( auto j=0; j<pF.m_nType; ++j ){
                        mIy.emplace_back(pF.m_piVertex[j]);
                    }
                }
                qDebug() << "Indix size : " << mIy.size() << " Vertex: " << mVy.size()
                         << " face : " << mMeshY->m_nFace;

                LBO lboX, lboY;
                lboX.AssignMesh(mMeshX.get());
                lboX.Construct();

                lboY.AssignMesh(mMeshY.get());
                lboY.Construct();

                mAx = lboX.AMatrix();
//                auto itend = L.cend();
//                for ( auto it = L.begin(); it != itend; ++it ){
//                    qDebug() << *it;
//                }
//                uint nVs = mMeshX->m_nVertex;
//                std::vector<std::vector<double>> P(nVs);

//                for ( uint i=0; i<nVs; ++i ){
//                    auto corrFile = tr("%1/v%2.txt").arg(corr).arg(i);
//                    QFile f(corrFile);
//                    if ( !f.open(QIODevice::ReadOnly)){
//                        qDebug() << "Open failed : " << corrFile;
//                        continue;
//                    }
//                    QTextStream in(&f);
//                    QString line;
//                    std::vector<double> _p;
//                    while (!in.atEnd()){
//                        line = in.readLine();
//                        _p.emplace_back(line.toDouble());
//                    }
//                    f.close();
//                    qDebug() << i << " - " << _p.size();
//                    P.emplace_back(_p);
//                }
                qDebug() << "Mesh loaded!";
            }  catch (const std::exception &e) {
                qWarning() << "Load *.off failed";
                qDebug() << e.what();
            } catch (...){
                qDebug() << "Unhandled error!";
            }
        });
    });

    return widget;
}

bool LP_SURFNet::Run()
{
    emit glUpdateRequest();
    return false;
}

bool LP_SURFNet::eventFilter(QObject *watched, QEvent *event)
{
    std::function<void(const QString &, const int &, const std::vector<double> &)> calCorrespondence;
    calCorrespondence = [this](const QString & path, const int &vid, const std::vector<double> &A){
        auto filename = tr("%1/v%2.txt").arg(path).arg(vid);
        QFile f(filename);
        if ( !f.open(QIODevice::ReadOnly)){
            return;
        }
        QTextStream in(&f);
        QString line;
        std::vector<double> p;
        while (!in.atEnd()){
            line = in.readLine();
            p.emplace_back(std::abs(line.toDouble()));  //Asolute
        }
        f.close();

//        std::transform(p.begin(), p.end(), p.begin(), [base=p.data(),&A](const double &v){
//            return v / A[&v - base];
//        });
        p[vid] *= A[vid];

        double sum = 0.0;
        for ( auto &v : p ){
            sum += v*v;
        }
        sum = 1.0/std::sqrt(sum);
        std::transform(p.begin(), p.end(), p.begin(), [&sum](const double &v){
            return v * sum;
        });

        auto mm = std::minmax_element(p.begin(), p.end());
        auto delta = *mm.second - *mm.first;
        qDebug() << "Delta of P : " << delta << ", [" << mm.second - p.begin()
                 << ", " << mm.first - p.begin() << "]";
        delta = 1.0/delta;
        //Normalize
        std::transform(p.begin(), p.end(), p.begin(), [delta,min=mm.first](const double &v){
            return (v - *min) * delta;
        });
        mP = std::move(p);
    };

    std::function<int(const QString &, const int &)> p2p;
    p2p = [this](const QString & path, const int &vid){
        auto filename = tr("%1/p2p.txt").arg(path);
        QFile f(filename);
        if ( !f.open(QIODevice::ReadOnly)){
            return -1;
        }
        QTextStream in(&f);
        QString line;
        std::vector<uint> p;
        while (!in.atEnd()){
            line = in.readLine();
            p.emplace_back(line.toUInt());  //Asolute
        }
        f.close();

//        std::transform(p.begin(), p.end(), p.begin(), [base=p.data(),&A](const double &v){
//            return v / A[&v - base];
//        });

        return int(p[vid]);
    };

    if ( QEvent::MouseButtonRelease == event->type()){
        auto e = static_cast<QMouseEvent*>(event);

        if ( e->button() == Qt::LeftButton ){
            if (mMeshY){

                //Select the entity vertices
                auto &&tmp = g_GLSelector->SelectPoints3D("Shade",
                                                    &mVy.front()[0],
                                                    mVy.size(),
                                                    e->pos(), true);

                if ( !tmp.empty()){
                    auto &&pt_id = tmp.front();
                    qDebug() << "Picked : " << tmp;
                    auto corrId = p2p(mCorrPath, pt_id);
                    if (Qt::ControlModifier & e->modifiers()){
                        if ( !mPoints.contains(pt_id)){
                            mPoints.insert(pt_id, mPoints.size());
                            mP2P.insert(pt_id, corrId);
                        }
                    }else if (Qt::ShiftModifier & e->modifiers()){
                        if ( mPoints.contains(pt_id)){
                            qDebug() << mPoints.take(pt_id);
                            mP2P.take(pt_id);
                        }
                    }else{
                        mPoints.clear();
                        mP2P.clear();
                        mPoints.insert(pt_id,0);
                        mP2P.insert(pt_id,corrId);
                    }

                    emit glUpdateRequest();

                    return true;
                }
            }
        }else if ( e->button() == Qt::RightButton ){
            mPoints.clear();
            mP2P.clear();
            emit glUpdateRequest();
            return true;
//            QStringList list;
//            mFeaturePoints.setStringList(list);
        }
    }

    return QObject::eventFilter(watched, event);
}

void LP_SURFNet::FunctionalRender_L(QOpenGLContext *ctx, QSurface *surf, QOpenGLFramebufferObject *fbo, const LP_RendererCam &cam, const QVariant &options)
{
    Q_UNUSED(surf)  //Mostly not used within a Functional.

    mCam = cam;
    if ( !mInitialized ){   //The OpenGL resources, e.g. Shader, not initilized
        initializeGL();     //Call the initialize member function.
    }                       //Not compulsory, (Using member function for cleaness only)
    if ( mVx.empty() || mVy.empty()){
        return;
    }

    auto proj = cam->ProjectionMatrix(),    //Get the projection matrix of the 3D view
         view = cam->ViewMatrix();          //Get the view matrix of the 3D view

    auto f = ctx->extraFunctions();         //Get the OpenGL functions container

    fbo->bind();
    f->glEnable(GL_PROGRAM_POINT_SIZE);     //Enable point-size controlled by shader
    f->glEnable(GL_DEPTH_TEST);             //Enable depth test

    auto view_ = view;
    view_.translate(QVector3D(-1.5f,0.0f,0.0f));

    mProgram->bind();                       //Bind the member shader to the context
    mProgram->setUniformValue("m4_mvp", proj * view_ );  //Set the Model-View-Projection matrix
    mProgram->setUniformValue("m4_view",view_);
    mProgram->setUniformValue("m3_normal", view_.normalMatrix());
    mProgram->setUniformValue("f_pointSize", 7.0f);     //Set the point-size which is enabled before
    mProgram->setUniformValue("v4_color", QVector4D( 0.3f, 0.6f, 0.1f, 0.6f )); //Set the point color


    mProgram->enableAttributeArray("a_pos");        //Enable the "a_pos" attribute buffer of the shader
    mProgram->enableAttributeArray("a_norm");

    mProgram->setAttributeArray("a_pos",mVx.data()); //Set the buffer data of "a_pos"
    mProgram->setAttributeArray("a_norm", mNx.data());

    f->glDrawElements(GL_TRIANGLES, mIx.size(),
                      GL_UNSIGNED_INT, mIx.data());

    QString compare = "Selected id:";
    if ( !mPoints.empty()){                         //If some vertices are picked and record in mPoints
        mProgram->setUniformValue("f_pointSize", 8.0f);    //Enlarge the point-size
        mProgram->setUniformValue("v4_color", QVector4D( 0.1f, 0.1f, 0.8f, 1.0f )); //Change to another color


        std::vector<uint> list(mPoints.size());
        std::vector<uint> list2(mPoints.size());
        for ( size_t i = 0; i<list.size(); ++i ){
            list[i] = (mPoints.begin()+i).key();
            compare += tr("%1 ").arg(list[i]);

            list2[i] = mP2P[list[i]];
        }

        // ONLY draw the picked vertices again
        f->glDrawElements(GL_POINTS, list.size(), GL_UNSIGNED_INT, list.data());

        mProgram->setUniformValue("f_pointSize", 13.0f);    //Enlarge the point-size
        mProgram->setUniformValue("v4_color", QVector4D( 0.6f, 0.15f, 0.15f, 1.0f )); //Change to another color

        // ONLY draw the picked vertices again
        f->glDrawElements(GL_POINTS, list2.size(), GL_UNSIGNED_INT, list2.data());

    }

    mProgram->setUniformValue("m4_mvp", proj * view );  //Set the Model-View-Projection matrix
    mProgram->setUniformValue("f_pointSize", 3.5f);    //Enlarge the point-size
    mProgram->setUniformValue("v4_color", QVector4D( 0.5f, 0.5f, 0.5f, 0.6f )); //Set the point color
    mProgram->setAttributeArray("a_pos",mVy.data()); //Set the buffer data of "a_pos"
    mProgram->setAttributeArray("a_norm",mNy.data()); //Set the buffer data of "a_pos"

    //f->glDrawArrays(GL_POINTS, 0, GLsizei(mVy.size()));    //Actually draw all the points
    f->glDrawElements(GL_TRIANGLES, mIy.size(),
                      GL_UNSIGNED_INT, mIy.data());

    if ( !mPoints.empty()){                         //If some vertices are picked and record in mPoints
        mProgram->setUniformValue("f_pointSize", 13.0f);    //Enlarge the point-size
        mProgram->setUniformValue("v4_color", QVector4D( 0.1f, 0.6f, 0.1f, 1.0f )); //Change to another color

        std::vector<uint> list(mPoints.size());
        for ( size_t i = 0; i<list.size(); ++i ){
            list[i] = (mPoints.begin()+i).key();
            compare += tr("%1 ").arg(list[i]);
        }
        // ONLY draw the picked vertices again
        f->glDrawElements(GL_POINTS, GLsizei(mPoints.size()), GL_UNSIGNED_INT, list.data());

    }

//    compare += tr(" %1 ").arg("Correspondence : ");
//    auto nps = mP.size();
//    if ( nps ){
//        mProgram->setUniformValue("f_pointSize", 15.0f);    //Enlarge the point-size

//        auto minmax = std::minmax_element(mP.begin(),mP.end());
//        auto minId = minmax.first - mP.begin();
//        auto maxId = minmax.second - mP.begin();

//        compare += tr("min:%1 max:%2").arg(minId).arg(maxId);

//        mProgram->setUniformValue("v4_color", QVector4D( 0.0f, 0.0f, 0.0f, 1.0f )); //Set the point color
//        f->glDrawArrays(GL_POINTS, minId, 1);    //Actually draw all the points

//        mProgram->setUniformValue("v4_color", QVector4D( 1.0f, 0.0f, 0.0f, 1.0f )); //Set the point color
//        f->glDrawArrays(GL_POINTS, maxId, 1);    //Actually draw all the points

//        mProgram->setUniformValue("f_pointSize", 5.0f);    //Enlarge the point-size

//        for ( size_t i=0; i < nps; ++i ){
//            mProgram->setUniformValue("v4_color", QVector4D( mP[i], 1.0-mP[i], 1.0-mP[i], 1.0f )); //Set the point color
//            f->glDrawArrays(GL_POINTS, i, 1);    //Actually draw all the points
//        }
//    }

//    qDebug() << compare;

    mProgram->disableAttributeArray("a_pos");   //Disable the "a_pos" buffer
    mProgram->disableAttributeArray("a_norm");
    mProgram->release();                        //Release the shader from the context

    fbo->release();
    f->glDisable(GL_PROGRAM_POINT_SIZE);
    f->glDisable(GL_DEPTH_TEST);
}

void LP_SURFNet::PainterDraw(QWidget *glW)
{
    if ( "window_Normal" == glW->objectName()){
        return;
    }
    if ( !mCam.lock() || mPoints.empty()){
        return;
    }
    auto cam = mCam.lock();
    auto view = cam->ViewMatrix(),
         proj = cam->ProjectionMatrix(),
         vp   = cam->ViewportMatrix();

    auto view2 = view;
    view2.translate(QVector3D(-1.5f,0.0f,0.0f));

    view = vp * proj * view;
    view2 = vp * proj * view2;

    auto &&h = cam->ResolutionY();
    QPainter painter(glW);
    int fontSize(13);
    QFont font("Arial", fontSize);
    QFontMetrics fmetric(font);
    QFont orgFont = painter.font();
    painter.setPen(qRgb(255,0,0));

    painter.setFont(font);

    for (auto it = mPoints.begin(); it != mPoints.end(); ++it ){
        auto vid = it.key();
        auto pickId = it.value();
        if ( vid > mVy.size()){
            continue;
        }
        auto pt = mVy[vid];
        QVector3D v(pt[0], pt[1], pt[2]);
        v = view * v;
        painter.drawText(QPointF(v.x()+5, h-v.y()), QString("%1").arg(pickId));

        pt = mVx[mP2P[vid]];
        QVector3D v2(pt[0], pt[1], pt[2]);
        v = view2 * v2;
        painter.drawText(QPointF(v.x()+5, h-v.y()), QString("%1").arg(pickId));
    }

    painter.setFont(orgFont);
}

void LP_SURFNet::initializeGL()
{
    constexpr char vsh[] =
            "attribute vec3 a_pos;\n"       //The position of a point in 3D that used in FunctionRender()
            "attribute vec3 a_norm;\n"
            "uniform mat4 m4_mvp;\n"        //The Model-View-Matrix
            "uniform mat4 m4_view;\n"
            "uniform mat3 m3_normal;\n"
            "uniform float f_pointSize;\n"  //Point size determined in FunctionRender()
            "varying vec3 normal;\n"
            "varying vec3 pos;\n"
            "void main(){\n"
            "   pos = vec3( m4_view * vec4(a_pos, 1.0));\n"
            "   normal = m3_normal * a_norm;\n"
            "   gl_Position = m4_mvp * vec4(a_pos, 1.0);\n" //Output the OpenGL position
            "   gl_PointSize = f_pointSize;\n"
            "}";
    constexpr char fsh[] =
            "uniform vec4 v4_color;\n"       //Defined the point color variable that will be set in FunctionRender()
            "varying vec3 pos;\n"
            "varying vec3 normal;\n"
            "void main(){\n"
            "   vec3 lightPos = vec3(0.0, 1000.0, 0.0);\n"
            "   vec3 viewDir = normalize( - pos);\n"
            "   vec3 lightDir = normalize(lightPos - pos);\n"
            "   vec3 H = normalize(viewDir + lightDir);\n"
            "   vec3 N = normalize(normal);\n"
            "   vec3 ambi = v4_color;\n"
            "   float Kd = max(dot(H, N), 0.0);\n"
            "   vec3 diff = Kd * vec3(0.5, 0.5, 0.5);\n"
            "   vec3 color = ambi + diff;\n"
            "   float Ks = pow( Kd, 80.0 );\n"
            "   vec3 spec = Ks * vec3(0.5, 0.5, 0.5);\n"
            "   color += spec;\n"
            "   gl_FragColor = vec4(color,1.0);\n"
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
