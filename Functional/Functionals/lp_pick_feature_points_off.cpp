#include "lp_pick_feature_points_off.h"


#include "lp_renderercam.h"
#include "lp_openmesh.h"
#include "renderer/lp_glselector.h"
#include "renderer/lp_glrenderer.h"

#include <QFileDialog>
#include <QVBoxLayout>
#include <QMouseEvent>
#include <QOpenGLContext>
#include <QOpenGLShaderProgram>
#include <QOpenGLExtraFunctions>
#include <QOpenGLFramebufferObject>
#include <QLabel>
#include <QMatrix4x4>
#include <QPushButton>
#include <QtConcurrent/QtConcurrent>
#include <QPainter>
#include <QPainterPath>

REGISTER_FUNCTIONAL_IMPLEMENT(LP_Pick_Feature_Points_OFF, Create Feature Points (OFF), menuTools)

LP_Pick_Feature_Points_OFF::LP_Pick_Feature_Points_OFF(QObject *parent) : LP_Functional(parent)
{

}

LP_Pick_Feature_Points_OFF::~LP_Pick_Feature_Points_OFF()
{
    emit glContextRequest([this](){
        delete mProgram;
        mProgram = nullptr;
    });
    Q_ASSERT(!mProgram);
}


QWidget *LP_Pick_Feature_Points_OFF::DockUi()
{
    mWidget = std::make_shared<QWidget>();

    QVBoxLayout *layout = new QVBoxLayout(mWidget.get());

    mLabel = new QLabel("Select A Mesh");
    QPushButton *button = new QPushButton(tr("Export"));
    QPushButton *impFts = new QPushButton(tr("Import"));
    QPushButton *importOFF = new QPushButton(tr("Import OFF"));
    QPushButton *mergeFts = new QPushButton(tr("Merge"));

    layout->addWidget(impFts);
    layout->addWidget(mergeFts);
    layout->addWidget(mLabel);
    layout->addWidget(button);
    layout->addWidget(importOFF);

    layout->addStretch();

    connect(mergeFts, &QPushButton::clicked,[this](){
        auto land1 = QFileDialog::getOpenFileName(0,"Import LandMarks1");
        if ( land1.isEmpty()){
            return;
        }
        auto land2 = QFileDialog::getOpenFileName(0,"Import LandMarks2");
        if ( land2.isEmpty()){
            return;
        }
        std::function<QMap<uint, uint>(const QString &filename)> func;
        func = [](const QString &filename){
            QMap<uint,uint> map;

            QFile file(filename);
            if ( !file.open(QIODevice::ReadOnly)){
                return map;
            }

            QTextStream in(&file);
            while (!in.atEnd()){
                auto line = in.readLine();
                auto items = line.split(' ');
                if ( 2 == items.size()){
                    map.insert(items[1].toUInt(), items[0].toUInt());
                }
            }

            file.close();

            return map;
        };
        auto &&map1 = func(land1);
        auto &&map2 = func(land2);
        if ( map1.isEmpty() || map2.isEmpty()){
            return;
        }
        if ( map1.size() != map2.size()){
            qWarning() << "Number of features mismatched";
            return;
        }
        QFile exp("../mergedLandmarks.txt");
        if ( !exp.open(QIODevice::WriteOnly)){
            return;
        }
        const int nFts = map1.size();
        QTextStream out(&exp);
        for ( int i=0; i<nFts; ++i ){
            out << (map1.begin()+i).value() << " " << (map2.begin()+i).value() << "\n";
        }

        exp.close();
    });

    connect(impFts, &QPushButton::clicked,[this](){
        auto openPath = QFileDialog::getOpenFileName(0,"Import LandMarks");
        if ( openPath.isEmpty()){
            return;
        }
        QFile file(openPath);
        if ( !file.open(QIODevice::ReadOnly)){
            return;
        }

        QTextStream in(&file);
        while (!in.atEnd()){
            auto line = in.readLine();
            auto items = line.split(' ');
            if ( 2 == items.size()){
                mPoints.insert(items[0].toUInt(), items[1].toUInt());
            }
        }

        emit glUpdateRequest();
        file.close();
    });

    connect(button, &QPushButton::clicked,
            [this](){
        auto nPts = int(mPoints.size());
        if ( 0 >= nPts ){
            return;
        }

        auto savePath = QFileDialog::getSaveFileName(0,"Export LandMarks");
        if ( savePath.isEmpty()){
            return;
        }
        QFile file(savePath);
        if (!file.open(QIODevice::WriteOnly)){
            return;
        }
        auto end = mPoints.end();
        QTextStream out(&file);
        for ( auto i=mPoints.begin(); i!=end; ++i ){
            out << i.key() << ' ' << i.value() << "\n";
        }

        file.close();
    });

    connect(importOFF, &QPushButton::clicked,
            [this](){
        auto filename = QFileDialog::getOpenFileName(0,"Import OFF");
        if ( filename.isEmpty()){
            return;
        }
        QFile file(filename);
        if (!file.open(QIODevice::ReadOnly)){
            return;
        }
        std::vector<QVector3D> pts;
        std::vector<uint> fs, es;
        QTextStream in(&file);
        auto line = in.readLine();
        if ( "OFF" != line ){
            qWarning() << "Invalid header";
            file.close();
            return;
        }
        line = in.readLine();
        auto sizes = line.split(' ');
        if ( sizes.size() != 3 ){
            qWarning() << "Invalid header : number of vertices/faces is wrong";
            file.close();
            return;
        }
        const size_t nVs = sizes[0].toInt();
        const size_t nFs = sizes[1].toInt();
        for ( size_t i=0; i<nVs && !in.atEnd(); ++i ){
            line = in.readLine();
            auto v = line.split(' ');
            if ( 3 > v.size()){
                continue;
            }
            pts.emplace_back(QVector3D{v[0].toFloat(),v[1].toFloat(),v[2].toFloat()});
        }
        if ( pts.size() != nVs ){
            qWarning() << "Invalid vertex count " << pts.size() << "/" << nVs;
            file.close();
            return;
        }
        for ( size_t i=0; i<nFs && !in.atEnd(); ++i ){
            line = in.readLine();
            auto f = line.split(' ');
            if ( 3 > f.size()){
                continue;
            }
            fs.emplace_back(f[1].toInt());
            fs.emplace_back(f[2].toInt());
            fs.emplace_back(f[3].toInt());

            es.emplace_back(f[1].toInt());
            es.emplace_back(f[2].toInt());
            es.emplace_back(f[2].toInt());
            es.emplace_back(f[3].toInt());
            es.emplace_back(f[3].toInt());
            es.emplace_back(f[1].toInt());
        }
        if ( fs.size() != nFs*3 ){
            qWarning() << "Invalid face count " << fs.size() << "/" << nFs*3;
            file.close();
            return;
        }

        file.close();
        mVs = std::move(pts);
        mFids = std::move(fs);
        mEdges = std::move(es);

        QVector3D bbmin({std::numeric_limits<float>::max(),
                        std::numeric_limits<float>::max(),
                        std::numeric_limits<float>::max()}),
                bbmax({-std::numeric_limits<float>::max(),
                      -std::numeric_limits<float>::max(),
                      -std::numeric_limits<float>::max()});


        for ( auto &o : mVs){
            bbmin.setX( qMin(bbmin.x(),o.x()));
            bbmin.setY( qMin(bbmin.y(),o.y()));
            bbmin.setZ( qMin(bbmin.z(),o.z()));

            bbmax.setX( qMax(bbmax.x(),o.x()));
            bbmax.setY( qMax(bbmax.y(),o.y()));
            bbmax.setZ( qMax(bbmax.z(),o.z()));
        }

        auto diag = (bbmax - bbmin).length();
        if ( std::isinf(diag)){
            return;
        }
        auto center = 0.5*(bbmax + bbmin);
        auto cam = mCam.lock();
        cam->setDiagonal(diag);
        cam->SetTarget(center);
        cam->RefreshCamera();

        qDebug() << filename << "read successfully";
    });

    mWidget->setLayout(layout);
    return mWidget.get();
}

bool LP_Pick_Feature_Points_OFF::Run()
{
    g_GLSelector->ClearSelected();
    emit glUpdateRequest();
    return false;
}


bool LP_Pick_Feature_Points_OFF::eventFilter(QObject *watched, QEvent *event)
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
            //Select the entity vertices
            if ( !mVs.empty()){
                auto &&tmp = g_GLSelector->SelectPoints3D("Shade",
                                                    &mVs.at(0)[0],
                                                    mVs.size(),
                                                    e->pos(), true);
                if ( !tmp.empty()){
                    auto &&pt_id = tmp.front();
                    qDebug() << "Picked : " << tmp;
                    if (Qt::ControlModifier & e->modifiers()){
                        if ( !mPoints.contains(pt_id)){
                            mPoints.insert(pt_id, mPoints.size());
                        }
                    }else if (Qt::ShiftModifier & e->modifiers()){
                        if ( mPoints.contains(pt_id)){
                            qDebug() << mPoints.take(pt_id);
                        }
                    }else{
                        mPoints.clear();
                        mPoints.insert(pt_id,0);
                    }

                    QString info("Picked Points:");
                    for ( auto &p : mPoints ){
                        info += tr("%1\n").arg(p);
                    }
                    mLabel->setText(info);

                    if (!mPoints.empty()){
    //                    QStringList pList;
    //                    int id=0;
    //                    for ( auto &p : mPoints ){
    //                        pList << QString("%5: %1 ( %2, %3, %4 )").arg(p,8)
    //                                 .arg(c->mesh()->points()[p][0],6,'f',2)
    //                                .arg(c->mesh()->points()[p][1],6,'f',2)
    //                                .arg(c->mesh()->points()[p][2],6,'f',2)
    //                                .arg(id++, 3);
    //                    }
    //                    mFeaturePoints.setStringList(pList);
                        emit glUpdateRequest();
                    }
                    return true;
                }
            }
        }else if ( e->button() == Qt::RightButton ){
            mLabel->setText("Select A Mesh");
            mVs.clear();
            mFids.clear();
            mPoints.clear();
            emit glUpdateRequest();
            return true;
//            QStringList list;
//            mFeaturePoints.setStringList(list);
        }
    }

    return QObject::eventFilter(watched, event);
}


void LP_Pick_Feature_Points_OFF::FunctionalRender(QOpenGLContext *ctx, QSurface *surf, QOpenGLFramebufferObject *fbo, const LP_RendererCam &cam, const QVariant &options)
{
    Q_UNUSED(surf)  //Mostly not used within a Functional.
    Q_UNUSED(options)   //Not used in this functional.

    mCam = cam;

    if ( !mInitialized ){   //The OpenGL resources, e.g. Shader, not initilized
        initializeGL();     //Call the initialize member function.
    }                       //Not compulsory, (Using member function for cleaness only)
    if ( mVs.empty()){
        return;             //If not mesh is picked, return. mObject is a weak pointer
    }                       //to a LP_OpenMesh.

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
    mProgram->setAttributeArray("a_pos",mVs.data()); //Set the buffer data of "a_pos"

    f->glDrawElements(GL_TRIANGLES, mFids.size(),
                      GL_UNSIGNED_INT,
                      mFids.data());    //Actually draw all the points

    mProgram->setUniformValue("v4_color", QVector4D( 0.2f, 0.2f, 0.2f, 1.0f )); //Set the point color
    f->glDrawElements(GL_LINES, mEdges.size(),
                      GL_UNSIGNED_INT,
                      mEdges.data());    //Actually draw all the points

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
    mProgram->disableAttributeArray("a_pos");   //Disable the "a_pos" buffer

    mProgram->release();                        //Release the shader from the context

    fbo->release();
    f->glDisable(GL_PROGRAM_POINT_SIZE);
    f->glDisable(GL_DEPTH_TEST);
}

void LP_Pick_Feature_Points_OFF::PainterDraw(QWidget *glW)
{
    if ( "openGLWidget_2" == glW->objectName()){
        return;
    }
    if ( !mCam.lock() || mVs.empty()){
        return;
    }
    auto cam = mCam.lock();
    auto view = cam->ViewMatrix(),
         proj = cam->ProjectionMatrix(),
         vp   = cam->ViewportMatrix();

    view = vp * proj * view;
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
        if ( vid > mVs.size()){
            continue;
        }
        auto pt = mVs[vid];
        QVector3D v(pt[0], pt[1], pt[2]);
        v = view * v;
        painter.drawText(QPointF(v.x(), h-v.y()), QString("%1").arg(pickId));
    }

    painter.setFont(orgFont);
}

void LP_Pick_Feature_Points_OFF::initializeGL()
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
