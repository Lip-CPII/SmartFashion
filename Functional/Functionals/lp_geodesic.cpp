#include "lp_geodesic.h"

#include "lp_renderercam.h"
#include "lp_openmesh.h"
#include "renderer/lp_glselector.h"
#include "renderer/lp_glrenderer.h"
#include <math.h>


#include <QVBoxLayout>
#include <QMouseEvent>
#include <QOpenGLContext>
#include <QOpenGLShaderProgram>
#include <QOpenGLExtraFunctions>
#include <QLabel>
#include <QMatrix4x4>
#include <QPushButton>
#include <QtConcurrent/QtConcurrent>

REGISTER_FUNCTIONAL_IMPLEMENT(LP_Geodesic, Create Geodesic Distance Field, menuTools)

LP_Geodesic::LP_Geodesic(QObject *parent) : LP_Functional(parent)
{

}

LP_Geodesic::~LP_Geodesic()
{
    emit glContextRequest([this](){
        delete mProgram_L;
        mProgram_L = nullptr;
    }, "Shade");
    emit glContextRequest([this](){
        delete mProgram_R;
        mProgram_R = nullptr;
    }, "Normal");
    Q_ASSERT(!mProgram_L);
    Q_ASSERT(!mProgram_R);
}

QWidget *LP_Geodesic::DockUi()
{
    mWidget = std::make_shared<QWidget>();

    QVBoxLayout *layout = new QVBoxLayout(mWidget.get());

    mObjectid = new QLabel("Object Index");
    mLabel = new QLabel("Select A Mesh");
    QPushButton *button = new QPushButton(tr("Export"));

    geocheckbox = new QCheckBox("Show Geodesic Distance Field");
    geocheckbox->setCheckable(1);
    connect(geocheckbox, SIGNAL(stateChanged(int)), this, SIGNAL(glUpdateRequest()));

    layout->addWidget(mLabel);
    layout->addWidget(mObjectid);
    layout->addWidget(button);
    layout->addWidget(geocheckbox);

    mWidget->setLayout(layout);
    return mWidget.get();
}

bool LP_Geodesic::Run()
{
    return false;
}

bool LP_Geodesic::eventFilter(QObject *watched, QEvent *event)
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
                auto Object = g_GLSelector->SelectInWorld("Shade",e->pos());
                if(Object.empty()){
                   return false;
                }
                auto c = _isMesh(Object.front());
                if ( c.lock()){
                    mObject = c;
                    mObjectid->setText(mObject.lock()->Uuid().toString());
                    mLabel->setText("Select the First Point");

                    if (!ocolor.empty()){
                        ocolor.clear();
                    }

                    emit glUpdateRequest();
                    return true;    //Since event filter has been called
                }

            }else{
                //Select the entity vertices


                auto c = _isMesh(mObject).lock();
                auto &&tmp = g_GLSelector->SelectPoints3D("Shade",
                                                    c->Mesh()->points()->data(),
                                                    c->Mesh()->n_vertices(),
                                                    e->pos(), true);

                if(mFirstPoint.empty()){
                    mLabel->setText("Select the First Point");
                }

                if(!mFirstPoint.empty() && !mSecondPoint.empty()){
                    mFirstPoint.clear();
                    mSecondPoint.clear();
                    Points.clear();
                    Faces.clear();
                    path_point.clear();
                    point_distance.clear();
                    length=0;
                    field_color.clear();
                    geocheckbox->setChecked(false);
                    emit glUpdateRequest();
                }

                else if(mFirstPoint.empty() && mSecondPoint.empty()){
                    mFirstPoint = std::move(tmp);
                    if(mFirstPoint.empty()){
                        return false;
                    }
                    mLabel->setText("Select the Second Point");
                    emit glUpdateRequest();
                }


                else if(!mFirstPoint.empty() && mSecondPoint.empty()){
                    geocheckbox->setChecked(false);
                    mSecondPoint = std::move(tmp);
                    if(mSecondPoint.empty()){
                        return false;
                    }


                    auto sf_mesh = c->Mesh();
                    auto sf_pt = sf_mesh->points();


                    Faces.resize(sf_mesh->n_faces()*3); // Triangular mesh
                    auto i_it = Faces.begin();
                    for ( const auto &f : sf_mesh->faces()){
                        for ( const auto &v : f.vertices()){
                            //Faces.emplace_back(v.idx());
                            (*i_it++) = v.idx();
                        }
                    }


                   for( int p = 0; p < (int)sf_mesh->n_vertices(); p++){
                        auto pp = sf_pt[p];
                        Points.push_back(pp[0]);
                        Points.push_back(pp[1]);
                        Points.push_back(pp[2]);
                    }



                    geodesic::Mesh mesh;
                    mesh.initialize_mesh_data(Points, Faces); // create internal mesh data structure including edges
                    geodesic::GeodesicAlgorithmExact algorithm(&mesh); // create exact algorithm for the mesh
                    geodesic::SurfacePoint source(&mesh.vertices()[mFirstPoint.front()]);
                    std::vector<geodesic::SurfacePoint> all_sources(1, source);

                    std::vector<geodesic::SurfacePoint> path;
                    geodesic::SurfacePoint target(&mesh.vertices()[mSecondPoint.front()]);
                    double const distance_limit = geodesic::GEODESIC_INF;
                    std::vector<geodesic::SurfacePoint> stop_points(1, target);
                    algorithm.propagate(all_sources, distance_limit, &stop_points);
                    algorithm.trace_back(target, path);



                    for( unsigned p_path=0; p_path < path.size(); p_path++){
                                geodesic::SurfacePoint& s = path[p_path];
                                path_point.emplace_back(s.x());
                                path_point.emplace_back(s.y());
                                path_point.emplace_back(s.z());
                    }



                      // Geodesic Distance Field

                    algorithm.propagate(path); // cover the whole mesh
                            for (unsigned i = 0; i < mesh.vertices().size(); ++i) {
                                geodesic::SurfacePoint p(&mesh.vertices()[i]);


                                double distance;
                                algorithm.best_source(p, distance); // for a given surface point, find closets source
                                                                                           // and distance to this source

//                                std::cout << i << ":" << distance << " "; // print geodesic distance for every vertex

                                point_distance.emplace_back(distance);

                                if(i == 0){
                                    length = distance;
                                }
                                else if(distance > length){
                                    length = distance;
                                }
                             }


                    for(int i=0; i< (int)sf_mesh->n_vertices(); i++){
                                float scale = point_distance[i]/length;
                                if(scale<0.5f){
                                    field_color.push_back(0.0f);
                                    field_color.push_back(1.0f-scale*2.0f);
                                    field_color.push_back(scale*2.0f);
                                }
                                if(scale==0.5f || scale>0.5f){
                                    field_color.push_back(scale*2.0f-1.0f);
                                    field_color.push_back(0.0f);
                                    field_color.push_back(1.0f/scale-1.0f);
                                }

                    }

                    mLabel->setText("Left or Right Click to Clear");

                    emit glUpdateRequest();
                }

                return true;
            }
        }else if ( e->button() == Qt::RightButton ){
            if(!mSecondPoint.empty()){
                mFirstPoint.clear();
                mSecondPoint.clear();
                Points.clear();
                Faces.clear();
                path_point.clear();
                point_distance.clear();
                length=0;
                field_color.clear();
                mLabel->setText("Left Click to Select the First Point\nRight Click to Reset");
                geocheckbox->setChecked(false);
                emit glUpdateRequest();
            }
            else{
                mFirstPoint.clear();
                mObject.reset();
                mLabel->setText("Select A Mesh");
                geocheckbox->setChecked(false);
                emit glUpdateRequest();
            }

//            QStringList list;
//            mFeaturePoints.setStringList(list);
        }
    }

    return QObject::eventFilter(watched, event);
}


void LP_Geodesic::FunctionalRender_L(QOpenGLContext *ctx, QSurface *surf, QOpenGLFramebufferObject *fbo, const LP_RendererCam &cam, const QVariant &options)
{

    Q_UNUSED(surf)  //Mostly not used within a Functional.
//    Q_UNUSED(options)   //Not used in this functional.


    if ( !mInitialized_L ){   //The OpenGL resources, e.g. Shader, not initilized
        initializeGL_L();     //Call the initialize member function.
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

    mProgram_L->bind();                       //Bind the member shader to the context
    mProgram_L->setUniformValue("m4_mvp", proj * view );  //Set the Model-View-Projection matrix
    mProgram_L->setUniformValue("f_pointSize", 7.0f);     //Set the point-size which is enabled before
    mProgram_L->setUniformValue("v4_color", QVector4D( 0.3f, 1.0f, 0.3f, 0.6f )); //Set the point color


    //Get the actual open-mesh data from the LP_OpenMesh class
    auto m = std::static_pointer_cast<LP_OpenMeshImpl>(mObject.lock())->Mesh();

    mProgram_L->enableAttributeArray("a_pos");        //Enable the "a_pos" attribute buffer of the shader
    mProgram_L->setAttributeArray("a_pos",m->points()->data(),3); //Set the buffer data of "a_pos"

    if ( point_distance.empty()){
        f->glDrawArrays(GL_POINTS, 0, GLsizei(m->n_vertices()));    //Actually draw all the points
    }


    if ( !mFirstPoint.empty()){
        mProgram_L->setUniformValue("f_pointSize", 13.0f);
        mProgram_L->setUniformValue("v4_color", QVector4D( 1.0f, 0.2f, 0.1f, 1.0f ));
        // ONLY draw the picked vertices again
        f->glDrawElements(GL_POINTS, GLsizei(mFirstPoint.size()), GL_UNSIGNED_INT, mFirstPoint.data());
    }

    if ( !mSecondPoint.empty()){
        mProgram_L->setUniformValue("f_pointSize", 13.0f);
        mProgram_L->setUniformValue("v4_color", QVector4D( 1.0f, 0.2f, 0.1f, 1.0f ));
        // ONLY draw the picked vertices again
        f->glDrawElements(GL_POINTS, GLsizei(mSecondPoint.size()), GL_UNSIGNED_INT, mSecondPoint.data());
    }

    if ( !path_point.empty()){
        mProgram_L->setAttributeArray("a_pos", path_point.data(), 3); //Set the buffer data of "a_pos"
        f->glLineWidth(10.0f);
        f->glDrawArrays(GL_LINE_STRIP, 0, path_point.size()/3);
    }

    if ( !field_color.empty() && geocheckbox->isChecked() ){   // Draw the distance field

//        for(int i=0; i< (int)m->n_vertices(); i++){
//            float r = field_color[i*3];
//            float g = field_color[i*3+1];
//            mProgram->setUniformValue("v4_color", QVector4D( r, g, 0.0f, 1.0f ));
//            auto *p = &i;
//            f->glDrawElements(GL_POINTS, GLsizei(1), GL_UNSIGNED_INT, p);
//        }


        auto mesh = std::static_pointer_cast<LP_OpenMeshImpl>(mObject.lock());

        QVariant opt = options;

        mesh->DrawCleanUp(ctx, surf);
        mesh->DrawSetup(ctx, surf, opt);

        typename OpMesh::ConstVertexIter vIt(m->vertices_begin());
        typename OpMesh::ConstVertexIter vEnd(m->vertices_end());

        if(ocolor.empty()){ // Save the original colors
        auto cptr = m->vertex_colors();

            for ( size_t i=0; i< m->n_vertices(); ++i, ++cptr ){

            const uchar *_p = cptr->data();

            ocolor.push_back(_p[0] / 255.0f);
            ocolor.push_back(_p[1] / 255.0f);
            ocolor.push_back(_p[2] / 255.0f);
            }
        }

        OpMesh::Color vcolor(0,0,0);
        int i = 0;
        for (vIt = m->vertices_begin(); vIt!=vEnd; ++vIt){
            vcolor[0] = field_color[i*3]*255.0f;
            vcolor[1] = field_color[i*3+1]*255.0f;
            vcolor[2] = field_color[i*3+2]*255.0f;
            m->set_color(*vIt, vcolor);
            i++;
        }
        ctx->makeCurrent(surf);

        if(geocolored == false){
            emit glUpdateRequest();
        }

        geocolored = true;
    }

    else if( !ocolor.empty() && (field_color.empty() || !geocheckbox->isChecked()) ){ // Draw the original colors


        auto mesh = std::static_pointer_cast<LP_OpenMeshImpl>(mObject.lock());

        QVariant opt = options;

        mesh->DrawCleanUp(ctx, surf);
        mesh->DrawSetup(ctx, surf, opt);

        typename OpMesh::ConstVertexIter vIt(m->vertices_begin());
        typename OpMesh::ConstVertexIter vEnd(m->vertices_end());

        OpMesh::Color vcolor(0,0,0);
        int i = 0;
            for (vIt = m->vertices_begin(); vIt!=vEnd; ++vIt){
                vcolor[0] = ocolor[i*3]*255.0f;
                vcolor[1] = ocolor[i*3+1]*255.0f;
                vcolor[2] = ocolor[i*3+2]*255.0f;
                m->set_color(*vIt, vcolor);
                i++;
            }
        ctx->makeCurrent(surf);

        if(geocolored == true){
            emit glUpdateRequest();
        }
        else if(field_color.empty()){
            ocolor.clear();
        }

        geocolored = false;

    }


    mProgram_L->disableAttributeArray("a_pos");   //Disable the "a_pos" buffer

    mProgram_L->release();                        //Release the shader from the context

    fbo->release();
    f->glDisable(GL_PROGRAM_POINT_SIZE);
    f->glDisable(GL_DEPTH_TEST);
}

void LP_Geodesic::FunctionalRender_R(QOpenGLContext *ctx, QSurface *surf, QOpenGLFramebufferObject *fbo, const LP_RendererCam &cam, const QVariant &options)
{
    Q_UNUSED(surf)  //Mostly not used within a Functional.
//    Q_UNUSED(options)   //Not used in this functional.


    if ( !mInitialized_R ){   //The OpenGL resources, e.g. Shader, not initilized
        initializeGL_R();     //Call the initialize member function.
    }                       //Not compulsory, (Using member function for cleaness only)
    if ( !mObject.lock()){
        return;             //If not mesh is picked, return. mObject is a weak pointer
    }                       //to a LP_OpenMesh.

    auto proj = cam->ProjectionMatrix(),    //Get the projection matrix of the 3D view
         view = cam->ViewMatrix();          //Get the view matrix of the 3D view

    auto f = ctx->extraFunctions();         //Get the OpenGL functions container

    if ( !field_color.empty() && geocheckbox->isChecked() ){   // Draw the distance field
        auto mesh = std::static_pointer_cast<LP_OpenMeshImpl>(mObject.lock());

        QVariant opt = options;
        mesh->DrawSetup(ctx, surf, opt);
    }   else if( !ocolor.empty() && (field_color.empty() || !geocheckbox->isChecked()) ){ // Draw the original colors
        auto mesh = std::static_pointer_cast<LP_OpenMeshImpl>(mObject.lock());

        QVariant opt = options;
        mesh->DrawSetup(ctx, surf, opt);
    }
    fbo->bind();
    f->glEnable(GL_PROGRAM_POINT_SIZE);     //Enable point-size controlled by shader
    f->glEnable(GL_DEPTH_TEST);             //Enable depth test

    mProgram_R->bind();                       //Bind the member shader to the context
    mProgram_R->setUniformValue("m4_mvp", proj * view );  //Set the Model-View-Projection matrix
    mProgram_R->setUniformValue("f_pointSize", 3.0f);     //Set the point-size which is enabled before


    //Get the actual open-mesh data from the LP_OpenMesh class
    auto m = std::static_pointer_cast<LP_OpenMeshImpl>(mObject.lock())->Mesh();

    mProgram_R->enableAttributeArray("a_pos");        //Enable the "a_pos" attribute buffer of the shader
    mProgram_R->setAttributeArray("a_pos",m->points()->data(),3); //Set the buffer data of "a_pos"

    if ( !mFirstPoint.empty()){
        mProgram_R->setUniformValue("f_pointSize", 8.0f);
        mProgram_R->setUniformValue("v4_color", QVector4D( 1.0f, 0.2f, 0.1f, 1.0f ));
        // ONLY draw the picked vertices again
        f->glDrawElements(GL_POINTS, GLsizei(mFirstPoint.size()), GL_UNSIGNED_INT, mFirstPoint.data());
    }

    if ( !mSecondPoint.empty()){
        mProgram_R->setUniformValue("f_pointSize", 8.0f);
        mProgram_R->setUniformValue("v4_color", QVector4D( 1.0f, 0.2f, 0.1f, 1.0f ));
        // ONLY draw the picked vertices again
        f->glDrawElements(GL_POINTS, GLsizei(mSecondPoint.size()), GL_UNSIGNED_INT, mSecondPoint.data());
    }

    if ( !path_point.empty()){
        mProgram_R->setAttributeArray("a_pos", path_point.data(), 3); //Set the buffer data of "a_pos"
        mProgram_R->setUniformValue("v4_color", QVector4D( 1.0f, 0.9f, 0.1f, 1.0f ));
        f->glLineWidth(5.0f);
        f->glDrawArrays(GL_LINE_STRIP, 0, path_point.size()/3);
    }


    mProgram_R->disableAttributeArray("a_pos");   //Disable the "a_pos" buffer

    mProgram_R->release();                        //Release the shader from the context

    fbo->release();
    f->glDisable(GL_PROGRAM_POINT_SIZE);
    f->glDisable(GL_DEPTH_TEST);
}

void LP_Geodesic::initializeGL_R()
{
    std::string vsh, fsh;

        vsh =
            "attribute vec3 a_pos;\n"       //The position of a point in 3D that used in FunctionRender()
            "uniform mat4 m4_mvp;\n"        //The Model-View-Matrix
            "uniform float f_pointSize;\n"  //Point size determined in FunctionRender()
            "void main(){\n"
            "   gl_Position = m4_mvp * vec4(a_pos, 1.0);\n" //Output the OpenGL position
            "   gl_PointSize = f_pointSize;\n"
            "}";
        fsh =
            "uniform vec4 v4_color;\n"       //Defined the point color variable that will be set in FunctionRender()
            "void main(){\n"
            "   gl_FragColor = v4_color;\n" //Output the fragment color;
            "}";

    auto prog = new QOpenGLShaderProgram;   //Intialize the Shader with the above GLSL codes
    prog->addShaderFromSourceCode(QOpenGLShader::Vertex,vsh.c_str());
    prog->addShaderFromSourceCode(QOpenGLShader::Fragment,fsh.data());
    if (!prog->create() || !prog->link()){  //Check whether the GLSL codes are valid
        qDebug() << prog->log();
        return;
    }

    mProgram_R = prog;            //If everything is fine, assign to the member variable

    mInitialized_R = true;
}

void LP_Geodesic::initializeGL_L()
{

        std::string vsh, fsh;

            vsh =
                "attribute vec3 a_pos;\n"       //The position of a point in 3D that used in FunctionRender()
                "uniform mat4 m4_mvp;\n"        //The Model-View-Matrix
                "uniform float f_pointSize;\n"  //Point size determined in FunctionRender()
                "void main(){\n"
                "   gl_Position = m4_mvp * vec4(a_pos, 1.0);\n" //Output the OpenGL position
                "   gl_PointSize = f_pointSize;\n"
                "}";
            fsh =
                "uniform vec4 v4_color;\n"       //Defined the point color variable that will be set in FunctionRender()
                "void main(){\n"
                "   gl_FragColor = v4_color;\n" //Output the fragment color;
                "}";

        auto prog = new QOpenGLShaderProgram;   //Intialize the Shader with the above GLSL codes
        prog->addShaderFromSourceCode(QOpenGLShader::Vertex,vsh.c_str());
        prog->addShaderFromSourceCode(QOpenGLShader::Fragment,fsh.data());
        if (!prog->create() || !prog->link()){  //Check whether the GLSL codes are valid
            qDebug() << prog->log();
            return;
        }

        mProgram_L = prog;            //If everything is fine, assign to the member variable

        mInitialized_L = true;
}

