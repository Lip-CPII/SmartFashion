#include "lp_mesh_slicer.h"
#include "lp_openmesh.h"
#include "lp_renderercam.h"

#include "renderer/lp_glselector.h"

#include "MeshPlaneIntersect.hpp"

#include <QAction>
#include <QSlider>
#include <QVBoxLayout>
#include <QMouseEvent>
#include <QOpenGLContext>
#include <QOpenGLShaderProgram>
#include <QOpenGLFramebufferObject>
#include <QOpenGLExtraFunctions>
#include <QDoubleSpinBox>
#include <QLabel>
#include <QMatrix4x4>
#include <QPushButton>
#include <QFileDialog>
#include <QMessageBox>
#include <QtConcurrent/QtConcurrent>

typedef MeshPlaneIntersect<float, int> Intersector;

std::vector<Intersector::Vec3D> vertices;
std::vector<Intersector::Face> faces;

LP_Mesh_Slicer::~LP_Mesh_Slicer()
{
    emit glContextRequest([this](){
        delete mProgram;
        mProgram = nullptr;
    });
    Q_ASSERT(!mProgram);
}

QString LP_Mesh_Slicer::MenuName()
{
    return tr("menuPlugins");
}

QWidget *LP_Mesh_Slicer::DockUi()
{
    mWidget = std::make_shared<QWidget>();
    auto widget = mWidget.get();
    QVBoxLayout *layout = new QVBoxLayout;

    mLabel = new QLabel("Select a mesh");
    QSlider *slider = new QSlider(widget);
    QDoubleSpinBox *dxspin = new QDoubleSpinBox;
    QPushButton *button = new QPushButton("Slice all");
    QPushButton *exportLoop = new QPushButton("Export Loop");   //Export Loop button

    dxspin->setRange(0,90);
    dxspin->setValue(0);

    slider->setRange(0,1000);
    slider->setTickInterval(1);

    layout->addWidget(mLabel);
    layout->addWidget(dxspin);
    layout->addWidget(slider);
    layout->addWidget(button);
    layout->addWidget(exportLoop);

    widget->setLayout(layout);

    connect(slider, &QSlider::valueChanged,
            this, &LP_Mesh_Slicer::sliderValueChanged);
    connect(dxspin, &QDoubleSpinBox::valueChanged,
            this, [this](const double &v){
        QVector3D norm(0.0f,1.0f,0.0);
        QMatrix4x4 rot;
        rot.rotate(v, QVector3D(1.0f,0.0f,0.0f));
        norm = rot * norm;
        mNormal[0] = norm.x();mNormal[1] = norm.y();mNormal[2] = norm.z();
    });
    connect(button, &QPushButton::clicked,
            this, [slider,this](bool clicked){
        Q_UNUSED(clicked)
        auto future = QtConcurrent::run(&mPool,[slider](){
            auto &&min = slider->minimum();
            auto &&max = slider->maximum();
            auto &&step = slider->singleStep();

            for ( auto i=min; i<max; i+=step){
                slider->setValue(i);
            }
        });
    });

    //Export all the loops to a .txt file
    connect(exportLoop, &QPushButton::clicked,
            this, [this](bool clicked){
        Q_UNUSED(clicked)
        if ( mPaths.empty()){
            QMessageBox::information(0,"Information","No path to export.");
            return;
        }
        auto filename = QFileDialog::getSaveFileName(0,"Save Loops","",tr("Text (*.txt)"));
        if ( filename.isEmpty()){   //If the user cancelled
            return;
        }

        //For @Issac 30-11-2020
        auto mesh = std::static_pointer_cast<LP_OpenMeshImpl>(mObject.lock());
        qDebug() << mesh->mFileName;

        auto future = QtConcurrent::run(&mPool,[this,filename](){
            //Ask user to provide a filename for saving the paths
            QFile file(filename);   //Get a file handler
            if ( !file.open(QIODevice::WriteOnly)){  //Open the file to write failed
                qDebug() << "Fail to open file: " << filename;
                return;
            }
            QString data;
            int loopCount = 0;
            for ( auto &layer : mPaths ){//For every "path" in "mPaths"
                int nLoops = layer.size();

                for ( int i=0; i<nLoops; ++i ){ //For all loops in that layer
                    data += tr("Loop %1\n").arg(loopCount++);   //Save the layer info.

                    auto &loop = layer.at(i);
                    for ( auto &v : loop ){ //For a loop in that layer
                        data += tr("%1 %2 %3\n").arg(v.x()).arg(v.y()).arg(v.z());  //Save the point x,y,z coordinates
                    }
                }
            }
            QTextStream out(&file); //Create a text-stream for writing data into the file
            out << data;        //Push the data into the file

            file.close();   //Close the file
        });
    });

    return widget;
}

bool LP_Mesh_Slicer::Run()
{
    mPool.setMaxThreadCount(std::max(1, mPool.maxThreadCount()-2));
    g_GLSelector->ClearSelected();
    return false;
}

QAction *LP_Mesh_Slicer::Trigger()
{
    if ( !mAction ){
        mAction = new QAction("Mesh Slicer");
    }
    return mAction;
}

bool LP_Mesh_Slicer::eventFilter(QObject *watched, QEvent *event)
{
    static auto _isMesh = [](LP_Objectw obj){
        if ( obj.expired()){
            return LP_OpenMeshw();
        }
        return LP_OpenMeshw() = std::static_pointer_cast<LP_OpenMeshImpl>(obj.lock());
    };

    if ( QEvent::MouseButtonRelease == event->type()){
        auto e = static_cast<QMouseEvent*>(event);

        if ( e->button() == Qt::LeftButton ){
            if (!mObject.lock()){
                auto &&objs = g_GLSelector->SelectInWorld("Shade",e->pos());
                for ( auto &o : objs ){
                    auto c_ = _isMesh(o);
                    if ( auto c = c_.lock()){
                        QVector3D minB, maxB;
                        c->BoundingBox(minB, maxB);

//                        mRange[0] = minB.y();
//                        mRange[1] = maxB.y();
                        mRange[0] = minB.z();
                        mRange[1] = maxB.z();

                        mObject = o;
                        mLabel->setText(mObject.lock()->Uuid().toString());

                        auto m = c->Mesh();
                        vertices.resize(m->n_vertices());
                        memcpy(vertices.data(), m->points(), m->n_vertices() * sizeof(OpMesh::Point));
                        faces.resize(m->n_faces());
                        auto it = faces.begin();

                        for ( auto &f : m->faces()){
                            Intersector::Face tmpF;
                            auto it_ = tmpF.begin();
                            for ( const auto &vx : f.vertices()){
                                *it_++ = vx.idx();
                            }
                            *it++ = std::move(tmpF);
                        }
                        break;
                    }
                }
            }
            return true;    //Since event filter has been called
        }else if ( e->button() == Qt::RightButton && !mMouseMove ){
            //g_GLSelector->Clear();
            mObject = LP_Objectw();
            mLabel->setText("Select A Mesh");
            mPaths.clear();
            emit glUpdateRequest();
        }
        mMouseMove = false;
    }else if (QEvent::MouseMove == event->type()){
        mMouseMove = true;
    }
    return QObject::eventFilter(watched, event);
}

void LP_Mesh_Slicer::FunctionalRender(QOpenGLContext *ctx, QSurface *surf, QOpenGLFramebufferObject *fbo, const LP_RendererCam &cam, const QVariant &options)
{
    Q_UNUSED(surf)
    Q_UNUSED(options)

    if ( !mInitialized ){
        initializeGL();
    }
    if ( !mObject.lock()){
        return;
    }
    QMatrix4x4 proj = cam->ProjectionMatrix(),
               view = cam->ViewMatrix();

    static auto rainbow = [](int p, int np, float&r, float&g, float&b) {
            float inc = 6.0 / np;
            float x = p * inc;
            r = 0.0f; g = 0.0f; b = 0.0f;
            if ((0 <= x && x <= 1) || (5 <= x && x <= 6)) r = 1.0f;
            else if (4 <= x && x <= 5) r = x - 4;
            else if (1 <= x && x <= 2) r = 1.0f - (x - 1);
            if (1 <= x && x <= 3) g = 1.0f;
            else if (0 <= x && x <= 1) g = x - 0;
            else if (3 <= x && x <= 4) g = 1.0f - (x - 3);
            if (3 <= x && x <= 5) b = 1.0f;
            else if (2 <= x && x <= 3) b = x - 2;
            else if (5 <= x && x <= 6) b = 1.0f - (x - 5);
        };

    auto f = ctx->extraFunctions();

    fbo->bind();

    f->glEnable( GL_DEPTH_TEST );
    f->glDepthFunc( GL_LEQUAL );
    f->glLineWidth(2.0f);
    constexpr auto sqrtEps = std::numeric_limits<float>::epsilon();
    f->glDepthRangef(-sqrtEps, 1.0f - sqrtEps);
    mProgram->bind();
    mProgram->setUniformValue("m4_mvp", proj * view );
    mProgram->setUniformValue("v4_color", QVector4D(1.0,1.0,0.0,1.0) );

    mProgram->enableAttributeArray("a_pos");
    if ( mLock.tryLockForRead()){

        const auto &nSections = mPaths.size();
        const auto &&keys = mPaths.keys();
       // qDebug() << nSections;
        for ( auto i=0; i<nSections; ++i ){
            const auto &key = keys.at(i);
            const auto &paths = mPaths[key];
            const auto &&nPaths = paths.size();
            for ( size_t j=0; j<nPaths; ++j ){
                auto &path = paths.at(j);
                //            QVector4D rgb(0.0,0.0,0.0,1.0);
                //            rainbow(i, nPaths, rgb[0], rgb[1], rgb[2] );
                //            mProgram->setUniformValue("v4_color", rgb );
                            mProgram->setAttributeArray("a_pos",path.data());

                            f->glDrawArrays(GL_LINE_STRIP, 0, GLsizei(path.size()));
            }
        }
        mLock.unlock();
    }

    mProgram->disableAttributeArray("a_pos");
    mProgram->release();
    f->glDisable(GL_DEPTH_TEST);
    f->glDepthFunc( GL_LESS );
    f->glDepthRangef(0.0f, 1.0f);
    fbo->release();
}

void LP_Mesh_Slicer::initializeGL()
{
    std::string vsh, fsh;
    vsh =
            "attribute vec3 a_pos;\n"
            "uniform mat4 m4_mvp;\n"
            "void main(){\n"
            "   gl_Position = m4_mvp * vec4(a_pos, 1.0);\n"
            "}";
    fsh =
            "uniform vec4 v4_color;\n"
            "void main(){\n"
            "   gl_FragColor = v4_color;\n"
            "}";

    auto prog = new QOpenGLShaderProgram;
    prog->addShaderFromSourceCode(QOpenGLShader::Vertex,vsh.c_str());
    prog->addShaderFromSourceCode(QOpenGLShader::Fragment,fsh.data());
    if (!prog->create() || !prog->link()){
        qDebug() << prog->log();
        return;
    }

    mProgram = prog;
    mInitialized = true;
}

void LP_Mesh_Slicer::sliderValueChanged(int v)
{
    if ( !mObject.lock()){
        return;
    }
    mLock.lockForRead();
    const auto it = mPaths.find(v);
    const auto itend = mPaths.end();
    mLock.unlock();
    if (it != itend){
        return;
    }

    auto future = QtConcurrent::run(&mPool,[this](const int &vv){

        auto height = mRange[0] + (vv * 1e-3f) * (mRange[1] - mRange[0]);

//        qDebug() << "Height : " << height;

        LP_OpenMeshw mm = std::static_pointer_cast<LP_OpenMeshImpl>(mObject.lock());
        auto m = mm.lock()->Mesh();
        if ( m ){
            // create the mesh object referencing the vertices and faces
            Intersector::Mesh mesh(vertices, faces);

            // by default the plane has an origin at (0,0,0) and normal (0,0,1)
            // these can be modified, but we can also just use the default
            Intersector::Plane plane;

            plane.normal = { mNormal[0], mNormal[1], mNormal[2] };
//            plane.origin = { 0.0f, height, 0.0f };
            plane.origin = { 0.0f, 0.0f, height };

            auto result = mesh.Intersect(plane);
            // the result is a vector of Path3D objects, which are planar polylines
            // in 3D space. They have an additional attribute 'isClosed' to determine
            // if the path forms a closed loop. if false, the path is open
            // in this case we expect one, open Path3D with one segment

            const auto nPaths = result.size();
            if ( 0 == nPaths ){
                return;
            }

            std::vector<std::vector<QVector3D>> paths;
            paths.resize( nPaths );

            for ( size_t i=0; i<nPaths; ++i ){
                const auto nPts = result.at(i).points.size();
                paths.at(i).resize( nPts );
                memcpy( paths.at(i).data(), result.at(i).points.data(), nPts * sizeof(QVector3D));
                //paths.at(i).emplace_back(paths.at(i).front());
            }
            mLock.lockForWrite();
            mPaths[vv] = std::move(paths);
            mLock.unlock();

            emit glUpdateRequest();
        }
    },v);
}
