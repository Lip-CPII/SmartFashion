#include "lp_mesh_slicer.h"
#include "lp_openmesh.h"
#include "lp_pointcloud.h"
#include "lp_renderercam.h"

#include "renderer/lp_glselector.h"

#include "MeshPlaneIntersect.hpp"

#include <flann/flann.hpp>
#include <opencv2/opencv.hpp>

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

struct LP_Mesh_Slicer::member {
    LP_OpenMesh mMesh;
    LP_PointCloud mPointCloud;

    std::multimap<float, const QVector3D *> mYSorted;

    void prepareMeshInfo(LP_Mesh_Slicer *functional);
    void preparePointCloudInfo(LP_Mesh_Slicer *functional);

    void sliceMesh(LP_Mesh_Slicer *functional, const int &vv, const float &height);
    void slicePointCloud(LP_Mesh_Slicer *functional, const int &vv, const float &height);
};


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
        QFileInfo infoOriginal(filename);
        QFileInfo info(mesh->mFileName);
        qDebug() << info.baseName();

        filename = tr("%1/%2.%3").arg(infoOriginal.dir().path())
                .arg(info.baseName())
                .arg(infoOriginal.suffix());
        qDebug() << filename;

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
    mMember = std::make_shared<member>();
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
    if ( QEvent::MouseButtonRelease == event->type()){
        auto e = static_cast<QMouseEvent*>(event);

        if ( e->button() == Qt::LeftButton ){
            if (!mObject.lock()){
                auto &&objs = g_GLSelector->SelectInWorld("Shade",e->pos());

                for ( auto &o : objs ){
                    if ( o.expired()){
                        continue;
                    }
                    if ( LP_OpenMeshImpl::mTypeName == o.lock()->TypeName()) {
                        mMember->mMesh = std::static_pointer_cast<LP_OpenMeshImpl>(o.lock());
                        mMember->prepareMeshInfo(this);
                        break;
                    } else if (LP_PointCloudImpl::mTypeName == o.lock()->TypeName()) {
                        mMember->mPointCloud = std::static_pointer_cast<LP_PointCloudImpl>(o.lock());
                        mMember->preparePointCloudInfo(this);
                        break;
                    } else {
                        continue;
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

void LP_Mesh_Slicer::FunctionalRender_L(QOpenGLContext *ctx, QSurface *surf, QOpenGLFramebufferObject *fbo, const LP_RendererCam &cam, const QVariant &options)
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
//                            QVector4D rgb(0.0,0.0,0.0,1.0);
//                            rainbow(i, nSections, rgb[0], rgb[1], rgb[2] );
//                            mProgram->setUniformValue("v4_color", rgb );
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

        if (mMember->mMesh){
            mMember->sliceMesh(this, vv, height );
        } else if (mMember->mPointCloud) {
            mMember->slicePointCloud(this, vv, height );
        } else {
            qDebug() << "No target selected !";
        }
    },v);
}

void LP_Mesh_Slicer::member::prepareMeshInfo(LP_Mesh_Slicer *functional)
{
    auto c = mMesh;
    QVector3D minB, maxB;
    c->BoundingBox(minB, maxB);

    functional->mRange[0] = minB.y();
    functional->mRange[1] = maxB.y();
//                        mRange[0] = minB.z();
//                        mRange[1] = maxB.z();

    functional->mObject = c;
    functional->mLabel->setText(c->Uuid().toString());

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
}

void LP_Mesh_Slicer::member::preparePointCloudInfo(LP_Mesh_Slicer *functional)
{
    auto pc = mPointCloud;
    QVector3D minB, maxB;
    pc->BoundingBox(minB, maxB);

    functional->mRange[0] = minB.y();
    functional->mRange[1] = maxB.y();

    functional->mObject = pc;
    functional->mLabel->setText(pc->Uuid().toString());

    mYSorted.clear();
    for (auto &p : pc->Points()){
        mYSorted.insert({p.y(), &p});
    }
}

void LP_Mesh_Slicer::member::sliceMesh(LP_Mesh_Slicer *functional, const int &vv, const float &height)
{
    LP_OpenMeshw mm = std::static_pointer_cast<LP_OpenMeshImpl>(functional->mObject.lock());
    auto m = mm.lock()->Mesh();
    if ( m ){
        // create the mesh object referencing the vertices and faces
        Intersector::Mesh mesh(vertices, faces);

        // by default the plane has an origin at (0,0,0) and normal (0,0,1)
        // these can be modified, but we can also just use the default
        Intersector::Plane plane;

        plane.normal = { functional->mNormal[0], functional->mNormal[1], functional->mNormal[2] };
        plane.origin = { 0.0f, height, 0.0f };
//            plane.origin = { 0.0f, 0.0f, height };

        auto result = mesh.Intersect(plane);
        // the result is a vector of Path3D objects, which are planar polylines
        // in 3D space. They have an additional attribute 'isClosed' to determine
        // if the path forms a closed loop. if false, the path is open
        // in this case we expect one, open Path3D with one segment

        const auto nPaths = result.size();
        if ( 0 == nPaths ){
            return;
        }
        auto copy = Intersector::Mesh::o_Edge;
        qDebug() << copy.front().front().first;

        std::vector<std::vector<QVector3D>> paths;
        paths.resize( nPaths );

        for ( size_t i=0; i<nPaths; ++i ){
            const auto nPts = result.at(i).points.size();
            paths.at(i).resize( nPts+1 );
            memcpy( paths.at(i).data(), result.at(i).points.data(), nPts * sizeof(QVector3D));
            paths.at(i)[nPts] = paths.at(i).front();
        }
        functional->mLock.lockForWrite();
        functional->mPaths[vv] = std::move(paths);
        functional->mLock.unlock();

        emit functional->glUpdateRequest();
    }
}

void LP_Mesh_Slicer::member::slicePointCloud(LP_Mesh_Slicer *functional, const int &vv, const float &height)
{
    if (!mPointCloud || mYSorted.empty()) {
        return;
    }

    std::multimap<float, cv::Point2f> xsorted;
    const float delta = 0.25f;
    float roi = 0.f;
    constexpr float point_scale = 1.0f;
    constexpr float point_scale_inv = 1.f / point_scale;
    int expandTimes = 0;
    while ( 3 > xsorted.size() && 1000 > ++expandTimes) {
        roi += delta;
        xsorted.clear();        //Redundant method
        auto itlow = mYSorted.lower_bound(height - roi),
             itup = mYSorted.upper_bound(height + roi);

        for (auto it = itlow; it != itup; ++it){
            xsorted.insert({it->second->x(), cv::Point2f(it->second->x(),it->second->z())});//On the xz-plane
        }
    }

    std::vector<std::vector<cv::Point>> contours;
    int cluster_id = -1;
    float lastX = -std::numeric_limits<float>::max();
    const float thres = 1.5f; //5unit
    for (auto it = xsorted.begin(); it != xsorted.end(); ++it){
        if ( thres < it->first - lastX ) {
            contours.resize(contours.size()+1);
            ++cluster_id;
        }
        contours.at(cluster_id).push_back(it->second * point_scale);
        lastX = it->first;
    }

    const int nClusters = contours.size();

    std::vector<std::vector<QVector3D>> loops(nClusters);

    for (int i=0; i<nClusters; ++i ) {
        if ( 3 > contours.at(i).size()) {
            continue;
        }
        auto &loop = loops.at(i);
        std::vector<cv::Point> hull;

        cv::convexHull(contours.at(i), hull);

//        //Smoothing
//        cv::Point bbmin(std::numeric_limits<int>::max(),
//                        std::numeric_limits<int>::max()),
//                  bbmax(-bbmin);

//        for ( auto &p : hull ) {
//            bbmin.x = cv::min(bbmin.x, p.x);
//            bbmin.y = cv::min(bbmin.y, p.y);
//            bbmax.x = cv::max(bbmax.x, p.x);
//            bbmax.y = cv::max(bbmax.y, p.y);
//        }
//        auto bbDelta = bbmax - bbmin;
//        float img_scale = 2.0f;
//        const auto offset = bbmin - 0.5 / img_scale * bbDelta;

//        std::vector<std::vector<cv::Point>> contours(1);
//        for ( auto &p : hull ){
//            contours[0].emplace_back(p - offset);
//        }

//        auto kernel_d(std::max(3, int(std::min(point_scale_inv*bbDelta.x, point_scale_inv*bbDelta.y))));

//        if ( 0 == kernel_d % 2 ) {
//            kernel_d += 1;  //Make odd
//        }
//        const cv::Size kernel(kernel_d, kernel_d);

//        std::vector<cv::Vec4i> hierachy;
//        cv::Mat mat = cv::Mat::zeros(img_scale*bbDelta.y, img_scale*bbDelta.x, CV_8UC1);
//        cv::drawContours(mat, contours, 0, cv::Scalar(255), -1);

//        cv::GaussianBlur(mat, mat, kernel,0);
//        cv::threshold(mat, mat, 100, 255, cv::THRESH_BINARY );

//        contours.clear();
//        cv::findContours(mat, contours, hierachy, cv::RETR_TREE, cv::CHAIN_APPROX_TC89_KCOS);

//        if ( contours.empty()) {
//            qWarning() << "No contour found ! " << i;
//            continue;
//        }

        for ( auto p : hull/*contours[0]*/ ) {
            //p += offset;
            p *= point_scale_inv;
            loop.emplace_back(p.x, height, p.y);
        }
        double length = 0.0;
        auto pre = loop.front();
        for ( int j=1; j < int(loop.size()); ++j ) {
            length += pre.distanceToPoint(loop.at(j));
            pre = loop.at(j);
        }
        qDebug() << "Length of loop : " << i << "=" << length;
        loop.push_back(loop.front());
    }

    functional->mLock.lockForWrite();
    functional->mPaths[vv] = loops;
    functional->mLock.unlock();

    emit functional->glUpdateRequest();
}
