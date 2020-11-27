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
                for ( uint i=0; i<mMeshX->m_nVertex; ++i ){
                    auto *p = &mMeshX->m_pVertex[i].m_vPosition;
                    mVx[i].setX(p->x);
                    mVx[i].setY(p->y);
                    mVx[i].setZ(p->z);
                }

                mMeshY = std::make_shared<CMesh>();
                if (!mMeshY->Load(fileY.toUtf8())){
                    mMeshY.reset();
                    qDebug() << "Load failed!";
                    return;
                }
                mVy.resize(mMeshY->m_nVertex);
                for ( uint i=0; i<mMeshY->m_nVertex; ++i ){
                    auto *p = &mMeshY->m_pVertex[i].m_vPosition;
                    mVy[i].setX(p->x);
                    mVy[i].setY(p->y);
                    mVy[i].setZ(p->z);
                }
                LBO lboX, lboY;
                lboX.AssignMesh(mMeshX.get());
                lboX.Construct();

                lboY.AssignMesh(mMeshX.get());
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

        p[vid] *= A[vid];

        auto max = *std::max_element(p.begin(), p.end());
        auto min = *std::min_element(p.begin(), p.end());
        auto delta = max - min;
        qDebug() << "Delta of P : " << delta;
        delta = 1.0/delta;
        //Normalize
        std::transform(p.begin(), p.end(), p.begin(), [delta](const double &v){
            return v * delta;
        });
        mP = std::move(p);
    };
    if ( QEvent::MouseButtonRelease == event->type()){
        auto e = static_cast<QMouseEvent*>(event);

        if ( e->button() == Qt::LeftButton ){
            if (mMeshX){

                //Select the entity vertices
                auto &&tmp = g_GLSelector->SelectPoints3D("Shade",
                                                    &mVx.front()[0],
                                                    mVx.size(),
                                                    e->pos(), true);
                if ( !tmp.empty()){
                    auto &&pt_id = tmp.front();
                    qDebug() << "Picked : " << tmp;
                    mPoints.clear();
                    mPoints.insert(pt_id,0);
                    calCorrespondence(mCorrPath, pt_id, mAx);

                    emit glUpdateRequest();

                    return true;
                }
            }
        }else if ( e->button() == Qt::RightButton ){
            mPoints.clear();
            emit glUpdateRequest();
            return true;
//            QStringList list;
//            mFeaturePoints.setStringList(list);
        }
    }

    return QObject::eventFilter(watched, event);
}

void LP_SURFNet::FunctionalRender(QOpenGLContext *ctx, QSurface *surf, QOpenGLFramebufferObject *fbo, const LP_RendererCam &cam, const QVariant &options)
{
    Q_UNUSED(surf)  //Mostly not used within a Functional.


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

    mProgram->bind();                       //Bind the member shader to the context
    mProgram->setUniformValue("m4_mvp", proj * view );  //Set the Model-View-Projection matrix
    mProgram->setUniformValue("f_pointSize", 7.0f);     //Set the point-size which is enabled before
    mProgram->setUniformValue("v4_color", QVector4D( 0.7f, 1.0f, 0.2f, 0.6f )); //Set the point color


    mProgram->enableAttributeArray("a_pos");        //Enable the "a_pos" attribute buffer of the shader
    mProgram->setAttributeArray("a_pos",mVx.data()); //Set the buffer data of "a_pos"
    f->glDrawArrays(GL_POINTS, 0, GLsizei(mVx.size()));    //Actually draw all the points

    if ( !mPoints.empty()){                         //If some vertices are picked and record in mPoints
        mProgram->setUniformValue("f_pointSize", 13.0f);    //Enlarge the point-size
        mProgram->setUniformValue("v4_color", QVector4D( 0.1f, 0.6f, 0.1f, 1.0f )); //Change to another color

        std::vector<uint> list(mPoints.size());
        for ( size_t i = 0; i<list.size(); ++i ){
            list[i] = (mPoints.begin()+i).key();
        }
        // ONLY draw the picked vertices again
        f->glDrawElements(GL_POINTS, GLsizei(mPoints.size()), GL_UNSIGNED_INT, list.data());
    }

    view.translate(QVector3D(1.5f,0.0f,0.0f));
    mProgram->setUniformValue("m4_mvp", proj * view );  //Set the Model-View-Projection matrix
    mProgram->setUniformValue("f_pointSize", 3.5f);    //Enlarge the point-size
    mProgram->setUniformValue("v4_color", QVector4D( 0.5f, 0.5f, 0.5f, 0.6f )); //Set the point color
    mProgram->setAttributeArray("a_pos",mVy.data()); //Set the buffer data of "a_pos"
    f->glDrawArrays(GL_POINTS, 0, GLsizei(mVy.size()));    //Actually draw all the points

    auto nps = mP.size();
    if ( nps ){
        mProgram->setUniformValue("f_pointSize", 5.0f);    //Enlarge the point-size
        for ( size_t i=0; i < nps; ++i ){
            mProgram->setUniformValue("v4_color", QVector4D( mP[i], 0.5f, 0.5f, 1.0f )); //Set the point color
            f->glDrawArrays(GL_POINTS, i, 1);    //Actually draw all the points
        }
    }


    mProgram->disableAttributeArray("a_pos");   //Disable the "a_pos" buffer

    mProgram->release();                        //Release the shader from the context

    fbo->release();
    f->glDisable(GL_PROGRAM_POINT_SIZE);
    f->glDisable(GL_DEPTH_TEST);
}

void LP_SURFNet::initializeGL()
{
    constexpr char vsh[] =
            "attribute vec3 a_pos;\n"       //The position of a point in 3D that used in FunctionRender()
            "uniform mat4 m4_mvp;\n"        //The Model-View-Matrix
            "uniform float f_pointSize;\n"  //Point size determined in FunctionRender()
            "void main(){\n"
            "   gl_Position = m4_mvp * vec4(a_pos, 1.0);\n" //Output the OpenGL position
            "   gl_PointSize = f_pointSize;\n"
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
