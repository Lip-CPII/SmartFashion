#include "lp_pick_feature_points.h"

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

REGISTER_FUNCTIONAL_IMPLEMENT(LP_Pick_Feature_Points, Create Feature Points, menuTools)

LP_Pick_Feature_Points::LP_Pick_Feature_Points(QObject *parent) : LP_Functional(parent)
{

}

LP_Pick_Feature_Points::~LP_Pick_Feature_Points()
{
    emit glContextRequest([this](){
        delete mProgram;
        mProgram = nullptr;
    });
    Q_ASSERT(!mProgram);
}


QWidget *LP_Pick_Feature_Points::DockUi()
{
    mWidget = std::make_shared<QWidget>();

    QVBoxLayout *layout = new QVBoxLayout(mWidget.get());

    mLabel = new QLabel("Select A Mesh");
    QPushButton *button = new QPushButton(tr("Export"));
    QPushButton *impFts = new QPushButton(tr("Import"));
    QPushButton *expFts = new QPushButton(tr("Export Pick Order"));

    layout->addWidget(impFts);
    layout->addWidget(mLabel);
    layout->addWidget(button);
    layout->addWidget(expFts);

    layout->addStretch();

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

    connect(expFts, &QPushButton::clicked,
            [this](){
        auto nPts = int(mPoints.size());
        if ( 0 >= nPts ){
            return;
        }

        auto savePath = QFileDialog::getSaveFileName(0,"Export LandMarks (Pick Order)");
        if ( savePath.isEmpty()){
            return;
        }
        QFile file(savePath);
        if (!file.open(QIODevice::WriteOnly)){
            return;
        }

        QMap<uint, uint> sorted;
        for( auto i = mPoints.begin(); i != mPoints.end(); ++i ){
            sorted.insert(i.value(), i.key());
        }
        auto end = sorted.end();
        QTextStream out(&file);
        for ( auto i=sorted.begin(); i!=end; ++i ){
            out << i.value() << "\n";
        }

        file.close();
    });

    mWidget->setLayout(layout);
    return mWidget.get();
}

bool LP_Pick_Feature_Points::Run()
{
    g_GLSelector->ClearSelected();
    return false;
}


bool LP_Pick_Feature_Points::eventFilter(QObject *watched, QEvent *event)
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
                    auto c = _isMesh(o);
                    if ( c.lock()){
                        mObject = o;
                        mLabel->setText(mObject.lock()->Uuid().toString());

                        emit glUpdateRequest();
                        return true;    //Since event filter has been called
                    }
                }
            }else{
                //Select the entity vertices

                auto c = _isMesh(mObject).lock();
                auto &&tmp = g_GLSelector->SelectPoints3D("Shade",
                                                    c->Mesh()->points()->data(),
                                                    c->Mesh()->n_vertices(),
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

                    QString info(mObject.lock()->Uuid().toString());
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
            mObject.reset();
            mLabel->setText("Select A Mesh");
            mPoints.clear();
            emit glUpdateRequest();
            return true;
//            QStringList list;
//            mFeaturePoints.setStringList(list);
        }
    }

    return QObject::eventFilter(watched, event);
}


void LP_Pick_Feature_Points::FunctionalRender(QOpenGLContext *ctx, QSurface *surf, QOpenGLFramebufferObject *fbo, const LP_RendererCam &cam, const QVariant &options)
{
    Q_UNUSED(surf)  //Mostly not used within a Functional.
    Q_UNUSED(options)   //Not used in this functional.

    if ( !mInitialized ){   //The OpenGL resources, e.g. Shader, not initilized
        initializeGL();     //Call the initialize member function.
    }                       //Not compulsory, (Using member function for cleaness only)
    if ( !mObject.lock()){
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

    //Get the actual open-mesh data from the LP_OpenMesh class
    auto m = std::static_pointer_cast<LP_OpenMeshImpl>(mObject.lock())->Mesh();

    mProgram->enableAttributeArray("a_pos");        //Enable the "a_pos" attribute buffer of the shader
    mProgram->setAttributeArray("a_pos",m->points()->data(),3); //Set the buffer data of "a_pos"

    f->glDrawArrays(GL_POINTS, 0, GLsizei(m->n_vertices()));    //Actually draw all the points

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

void LP_Pick_Feature_Points::initializeGL()
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
