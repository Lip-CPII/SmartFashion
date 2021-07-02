#include "sg_point_cloud_postprocess.h"
#include <iostream> //标准输入输出流

#include "lp_renderercam.h"
#include "lp_import_openmesh.h"
#include "Commands/lp_commandmanager.h"
#include "Commands/lp_cmd_import_opmesh.h"
#include "lp_pick_feature_points.h"
#include "renderer/lp_glselector.h"
#include "renderer/lp_glrenderer.h"
#include <fstream>
#include "lp_document.h"

#include <QMessageBox>
#include <QGroupBox>
#include <QMouseEvent>
#include <QOpenGLFramebufferObject>
#include <QOpenGLContext>
#include <QOpenGLShaderProgram>
#include <QOpenGLExtraFunctions>
#include <QOpenGLTexture>
#include <QOpenGLBuffer>
#include <QVBoxLayout>
#include <QLabel>
#include <QComboBox>
#include <QMatrix4x4>
#include <QPushButton>
#include <QFileDialog>
#include <QSlider>
#include <QCheckBox>
#include <QPainter>
#include <QPainterPath>
#include <QtConcurrent/QtConcurrent>

REGISTER_FUNCTIONAL_IMPLEMENT(sg_point_cloud_postprocess, PC_postprocess, menuPC)


sg_point_cloud_postprocess::sg_point_cloud_postprocess(QObject *parent) : LP_Functional(parent)
  ,mInitialized(false)
{

}


bool sg_point_cloud_postprocess::Run()
{
    auto it = g_Renderers.begin();    //Pick any renderer since all contexts are shared
    Q_ASSERT(it == g_Renderers.cbegin());
    auto renderer = it.value();
    Q_ASSERT(renderer);

    QMetaObject::invokeMethod(renderer,
                              "clearScene",
                              Qt::BlockingQueuedConnection);
    QMetaObject::invokeMethod(g_GLSelector.get(),
                              "ClearSelected");
    /**
     * @brief Clear the document container. Should be called after
     * clearing up the GL resources
     */
//    auto pDoc = &LP_Document::gDoc;
//    pDoc->ResetDocument();

//    emit pDoc->updateTreeView();
    /**
     * @brief Clear the stack at last since the actual objects
     * are created and stored in the COMMANDs that generates it.
     * Remarks: Event COMMANDs removing objects from "Document"
     * should not delete the object instantly because undoing
     * a delete-COMMAND does not know how the objects are created.
     */
    auto stack = LP_CommandGroup::gCommandGroup->ActiveStack();
    stack->clear();

    LP_GLRenderer::UpdateAll();

    return true;
}

void sg_point_cloud_postprocess::FunctionalRender_L(QOpenGLContext *ctx, QSurface *surf, QOpenGLFramebufferObject *fbo, const LP_RendererCam &cam, const QVariant &options)
{
    Q_UNUSED(surf)  //Mostly not used within a Functional.
    Q_UNUSED(options)   //Not used in this functional.

    mCam = cam;

    if ( !mInitialized ){   //The OpenGL resources, e.g. Shader, not initilized
        initializeGL();     //Call the initialize member function.
    }                       //Not compulsory, (Using member function for cleaness only)
//    if ( !mObject.lock()){
//        return;             //If not mesh is picked, return. mObject is a weak pointer
//    }                       //to a LP_OpenMesh.
    if(cloud_buffer.size()==0) return;
    auto proj = cam->ProjectionMatrix(),    //Get the projection matrix of the 3D view
         view = cam->ViewMatrix();          //Get the view matrix of the 3D view

    auto f = ctx->extraFunctions();         //Get the OpenGL functions container

    fbo->bind();
    f->glEnable(GL_PROGRAM_POINT_SIZE);     //Enable point-size controlled by shader
    f->glEnable(GL_DEPTH_TEST);             //Enable depth test

    mProgram->bind();                       //Bind the member shader to the context
    mProgram->setUniformValue("m4_mvp", proj * view );  //Set the Model-View-Projection matrix
    mProgram->setUniformValue("f_pointSize", 1.0f);     //Set the point-size which is enabled before
    mProgram->setUniformValue("v4_color", QVector4D( 0.4f, 0.7f, 0.7f, 0.6f )); //Set the point color

//    Get the actual open-mesh data from the LP_OpenMesh class
//    auto m = std::static_pointer_cast<LP_OpenMeshImpl>(mObject.lock())->Mesh();

    std::vector<float> cloudData;
    for(auto a = cloud_buffer[cloud_buffer.size()-1]->points.begin();a !=cloud_buffer[cloud_buffer.size()-1]->points.end();a++)
    {
        cloudData.push_back(a->_PointXYZRGB::x);
        cloudData.push_back(a->_PointXYZRGB::y);
        cloudData.push_back(a->_PointXYZRGB::z);
        mProgram->setUniformValue("v4_color", QVector4D( a->_PointXYZRGB::r/255.0f, a->_PointXYZRGB::g/255.0f, a->_PointXYZRGB::b/255.0f, 0.6f ));
        mProgram->enableAttributeArray("a_pos");        //Enable the "a_pos" attribute buffer of the shader
        mProgram->setAttributeArray("a_pos",cloudData.data(),3); //Set the buffer data of "a_pos"
        f->glDrawArrays(GL_POINTS, 0, GLsizei(1));
        cloudData.clear();
    }


    mProgram->disableAttributeArray("a_pos");   //Disable the "a_pos" buffer

    mProgram->release();                        //Release the shader from the context

    fbo->release();
    f->glDisable(GL_PROGRAM_POINT_SIZE);
    f->glDisable(GL_DEPTH_TEST);

}

QWidget *sg_point_cloud_postprocess::DockUi()
{
    mWidget = std::make_shared<QWidget>();

    QVBoxLayout *layout = new QVBoxLayout(mWidget.get());
    layout->setContentsMargins(0,2,0,2);
    QPushButton *buttonImportPLY = new QPushButton(tr("Import PLY File"),mWidget.get());
    layout->addWidget(buttonImportPLY);
    QGroupBox *groupA = new QGroupBox;
    QPushButton *buttonUndo = new QPushButton(tr("Undo"),mWidget.get());
    QGridLayout *glayoutA = new QGridLayout;
    glayoutA->addWidget(buttonUndo,0,0);
    labelBufferLength = new QLabel(tr("1"),mWidget.get());
    labelBufferLength->setAlignment(Qt::AlignHCenter|Qt::AlignVCenter);
    glayoutA->addWidget(labelBufferLength,0,1);
    glayoutA->setContentsMargins(5,2,5,2);
    groupA->setLayout(glayoutA);
    layout->addWidget(groupA);

    QGroupBox *groupB = new QGroupBox;
    QGridLayout *glayoutB = new QGridLayout;
    QPushButton *buttonResample = new QPushButton(tr("Resample"),mWidget.get());
    buttonResample->setMaximumWidth(100);
    glayoutB->addWidget(buttonResample,0,0);
    glayoutB->setContentsMargins(0,2,0,2);
    resample_x->setMaximumWidth(45);
    resample_x->setValue(1.0);
    resample_x->setSingleStep(0.1);
    glayoutB->addWidget(resample_x,0,1);
    resample_y->setMaximumWidth(45);
    resample_y->setValue(1.0);
    resample_y->setSingleStep(0.1);
    glayoutB->addWidget(resample_y,0,2);
    resample_z->setMaximumWidth(45);
    resample_z->setValue(1.0);
    resample_z->setSingleStep(0.1);

    glayoutB->addWidget(resample_z,0,3);

    QPushButton *buttonFilter_ROR = new QPushButton(tr("Filter_ROR"),mWidget.get());
    buttonFilter_ROR->setMaximumWidth(100);
    buttonFilter_ROR->setContentsMargins(2,0,2,0);
    glayoutB->addWidget(buttonFilter_ROR,1,0);
    ROR_p1->setMaximumWidth(45);
    ROR_p1->setValue(2.0);
    glayoutB->addWidget(ROR_p1,1,1);
    ROR_p2->setMaximumWidth(45);
    ROR_p2->setValue(4.0);
    glayoutB->addWidget(ROR_p2,1,2);

    QPushButton *buttonFilter_statistic = new QPushButton(tr("Filter_statistic"),mWidget.get());
    buttonFilter_statistic->setMaximumWidth(100);
    buttonFilter_statistic->setMinimumWidth(100);
    buttonFilter_statistic->setContentsMargins(2,0,2,0);
    glayoutB->addWidget(buttonFilter_statistic,2,0);

    statistic_p1->setMaximumWidth(45);
    statistic_p1->setValue(100);
    glayoutB->addWidget(statistic_p1,2,1);
    statistic_p2->setMaximumWidth(45);
    statistic_p2->setValue(0.5);
    glayoutB->addWidget(statistic_p2,2,2);

    groupB->setLayout(glayoutB);
    layout->addWidget(groupB);

    QPushButton *buttonExportPLY = new QPushButton(tr("ExportPLY"),mWidget.get());
    layout->addWidget(buttonExportPLY);
    layout->addStretch();
    mWidget->setLayout(layout);

    connect(buttonImportPLY, &QPushButton::clicked,[this](){
        importPLY();
        emit glUpdateRequest();
        });
    connect(buttonUndo,&QPushButton::clicked,[this](){
        if(cloud_buffer.size()>1)
        {
            cloud_buffer.pop_back();
            labelBufferLength->setText(QString::number(cloud_buffer.size()));
        }
        emit glUpdateRequest();
    });
    connect(buttonResample, &QPushButton::clicked,[this](){
        resample();
        labelBufferLength->setText(QString::number(cloud_buffer.size()));
        emit glUpdateRequest();
        });
    connect(buttonFilter_ROR, &QPushButton::clicked,[this](){
        filter_ROR();
        labelBufferLength->setText(QString::number(cloud_buffer.size()));
        emit glUpdateRequest();
        });
    connect(buttonFilter_statistic, &QPushButton::clicked,[this](){
        filter_statistic();
        labelBufferLength->setText(QString::number(cloud_buffer.size()));
        emit glUpdateRequest();
        });
    connect(buttonExportPLY, &QPushButton::clicked,[this](){
        emit glUpdateRequest();

    });

    return mWidget.get();
}

void sg_point_cloud_postprocess::initializeGL()
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

bool sg_point_cloud_postprocess::eventFilter(QObject *watched, QEvent *event)
{
//    static auto _isMesh = [](LP_Objectw obj){
//        if ( obj.expired() || LP_OpenMeshImpl::mTypeName != obj.lock()->TypeName()){
//            return LP_OpenMeshw();
//        }
//        return LP_OpenMeshw() = std::static_pointer_cast<LP_OpenMeshImpl>(obj.lock());
//    };

//    if ( QEvent::MouseButtonRelease == event->type()){
//        auto e = static_cast<QMouseEvent*>(event);

//        if ( e->button() == Qt::LeftButton ){
//            //Select the entity vertices
//            if ( !mVs.empty()){
//                auto rb = g_GLSelector->RubberBand();

//                qDebug() << rb->pos();
//                auto &&tmp = g_GLSelector->SelectPoints3D("Shade",
//                                                    &mVs.at(0)[0],
//                                                    mVs.size(),
//                                                    rb->pos()+rb->rect().center(), false,
//                                                    rb->width(), rb->height());
//                if ( !tmp.empty()){
//                    for (auto pt_id : tmp ){
//                        if (Qt::ControlModifier & e->modifiers()){
//                            if ( !mPoints.contains(pt_id)){
//                                mPoints.insert(pt_id, mPoints.size());
//                            }
//                        }else if (Qt::ShiftModifier & e->modifiers()){
//                            if ( mPoints.contains(pt_id)){
//                                qDebug() << mPoints.take(pt_id);
//                            }
//                        }else{
//                            mPoints.clear();
//                            mPoints.insert(pt_id,0);
//                        }
//                    }

//                    QString info("Picked Points:");
//                    for ( auto &p : mPoints ){
//                        info += tr("%1\n").arg(p);
//                    }
//                    mLabel->setText(info);

//                    if (!mPoints.empty()){
//    //                    QStringList pList;
//    //                    int id=0;
//    //                    for ( auto &p : mPoints ){
//    //                        pList << QString("%5: %1 ( %2, %3, %4 )").arg(p,8)
//    //                                 .arg(c->mesh()->points()[p][0],6,'f',2)
//    //                                .arg(c->mesh()->points()[p][1],6,'f',2)
//    //                                .arg(c->mesh()->points()[p][2],6,'f',2)
//    //                                .arg(id++, 3);
//    //                    }
//    //                    mFeaturePoints.setStringList(pList);
//                        emit glUpdateRequest();
//                    }
//                    return true;
//                }
//            } else {
//                auto &&tmp = g_GLSelector->SelectInWorld("Shade",
//                                                        e->pos());
//                for ( auto &o : tmp ){
//                    if ( o.lock() && LP_PointCloudImpl::mTypeName == o.lock()->TypeName()) {
//                        mObject = o;
//                        auto pc = std::static_pointer_cast<LP_PointCloudImpl>(o.lock());
//                        mVs = pc->Points();
//                        mNs = pc->Normals();
//                        LP_Document::gDoc.RemoveObject(std::move(o));
//                        emit glUpdateRequest();
//                        return true;    //Since event filter has been called
//                    }
//                }
//            }
//        }else if ( e->button() == Qt::RightButton ){
//            mLabel->setText("Select A Mesh");
//            mVs.clear();
//            mNs.clear();
//            mFids.clear();
//            mPoints.clear();
//            emit glUpdateRequest();
//            return true;
////            QStringList list;
////            mFeaturePoints.setStringList(list);
//        }
//    } else if (QEvent::KeyPress == event->type()){
//        QKeyEvent *e = static_cast<QKeyEvent*>(event);
//        switch(e->key()){
//        case Qt::Key_Delete:{
//            auto pc = std::static_pointer_cast<LP_PointCloudImpl>(mObject.lock());
//            const int nVs = mVs.size();
//            std::vector<QVector3D> newVs, newNs;
//            for ( int i=0; i<nVs; ++i ){
//                auto it = mPoints.find(i);
//                if ( it == mPoints.end()) {
//                    if ( qFabs(mVs[i].z()) > 1.3f ) {
//                        continue;
//                    }
//                    newVs.emplace_back(mVs[i]);
//                    newNs.emplace_back(mNs[i]);
//                }
//            }
//            mVs = newVs;
//            mNs = newNs;
//            mPoints.clear();
//            emit glUpdateRequest();
//            break;
//        }
//        case Qt::Key_Space:
//        case Qt::Key_Enter:{
//            auto pc = std::static_pointer_cast<LP_PointCloudImpl>(mObject.lock());
//            pc->SetPoints(std::move(mVs));
//            pc->SetNormals(std::move(mNs));
//            mVs.clear();
//            mNs.clear();
//            mPoints.clear();
//            LP_Document::gDoc.AddObject(std::move(mObject));
//            emit glUpdateRequest();
//            break;
//            break;
//        }
//        }
//    }

    return QObject::eventFilter(watched, event);
}

bool sg_point_cloud_postprocess::importPLY()
{
    auto file = QFileDialog::getOpenFileName(0,tr("Import Point Cloud"), "",
                                             tr("PointCloud(*.ply)"));

    if ( file.isEmpty()){
        return false;
    }
    qDebug()<<"Start import PLY File";

    pcl::PointCloud<pcl::PointXYZRGB>::Ptr cloud(new pcl::PointCloud<pcl::PointXYZRGB>);
    if (pcl::io::loadPLYFile<pcl::PointXYZRGB>(file.toStdString(), *cloud) == -1) //* 读入PCD格式的文件，如果文件不存在，返回-1
    {
        PCL_ERROR("Couldn't read file test_pcd.pcd \n"); //文件不存在时，返回错误，终止程序。
        return (false);
    }    
    cloud_buffer.emplace_back(cloud);
    qDebug()<<"The amount of oringal PointCloud is :"<<cloud_buffer.back()->size();



//-------------Filter3: FastBilateralFilter(unavaliable)----------------//
//    pcl::FastBilateralFilter<pcl::PointXYZRGB> filter_fbf;
//    filter_fbf.setInputCloud (cloud);
//    filter_fbf.setSigmaS (0.05);//设置双边滤波器用于空间邻域/窗口的高斯的标准偏差
//    filter_fbf.setSigmaR (0.05);//设置高斯的标准偏差用于控制相邻像素由于强度差异而下降多少（在我们的情况下为深度）
//    filter_fbf.filter(*cloud_filtered);

    return true;
}

bool sg_point_cloud_postprocess::resample()
{
    if(cloud_buffer.size()==0)
    {
        qDebug()<<"ERROR: NO POINTCLOUD DETECTED!";
        return false;
    }
    auto cloud = cloud_buffer.back();
    pcl::PointCloud<pcl::PointXYZRGB>::Ptr cloud_filtered(new pcl::PointCloud<pcl::PointXYZRGB>);
    pcl::VoxelGrid<pcl::PointXYZRGB> downSampled;
    downSampled.setInputCloud (cloud);
    downSampled.setLeafSize (resample_x->value(), resample_y->value(), resample_z->value());
    downSampled.filter (*cloud_filtered);
    cloud_buffer.emplace_back(cloud_filtered);
    if(cloud_buffer.back()->size()==0)
    {
        cloud_buffer.pop_back();
        qDebug()<<"The PointCloud is empty, please double-check the parameters";
        return false;
    }
    if(cloud_buffer.back()->size()==cloud_buffer[cloud_buffer.size()-2]->size())
    {
        cloud_buffer.pop_back();
        qDebug()<<"The filter is useless, please double-check the parameters";
        return false;
    }
    qDebug()<<"The amount of resampled PointCloud is :"<<cloud_buffer.back()->size();
    return true;
}

bool sg_point_cloud_postprocess::filter_ROR()
{
    if(cloud_buffer.size()==0)
    {
        qDebug()<<"ERROR: NO POINTCLOUD DETECTED!";
        return false;
    }
    else
    {
        auto cloud = cloud_buffer.back();
        pcl::PointCloud<pcl::PointXYZRGB>::Ptr cloud_filtered(new pcl::PointCloud<pcl::PointXYZRGB>);
        pcl::RadiusOutlierRemoval<pcl::PointXYZRGB> filter_ror;
        filter_ror.setInputCloud (cloud);
        filter_ror.setRadiusSearch(ROR_p1->value());               // 设置搜索半径
        filter_ror.setMinNeighborsInRadius(ROR_p2->value());      // 设置一个内点最少的邻居数目
        filter_ror.filter (*cloud_filtered);
        cloud_buffer.emplace_back(cloud_filtered);
        if(cloud_buffer.back()->size()==0)
        {
            cloud_buffer.pop_back();
            qDebug()<<"The PointCloud is empty, please double-check the parameters";
            return false;
        }
        if(cloud_buffer.back()->size()==cloud_buffer[cloud_buffer.size()-2]->size())
        {
            cloud_buffer.pop_back();
            qDebug()<<"The filter is useless, please double-check the parameters";
            return false;
        }
        qDebug()<<"The amount of ROR filtered PointCloud is :"<<cloud_buffer.back()->size();
    }
    return true;
}

bool sg_point_cloud_postprocess::filter_statistic()
{
    if(cloud_buffer.empty())
    {
        qDebug()<<"ERROR: NO POINTCLOUD DETECTED!";
        return false;
    }
    auto cloud = cloud_buffer.back();
    pcl::PointCloud<pcl::PointXYZRGB>::Ptr cloud_filtered(new pcl::PointCloud<pcl::PointXYZRGB>);
    pcl::StatisticalOutlierRemoval<pcl::PointXYZRGB> filter_statistic;   //创建滤波器对象
    filter_statistic.setInputCloud (cloud);                           //设置待滤波的点云
    filter_statistic.setMeanK (statistic_p1->value());                               //设置在进行统计时考虑的临近点个数
    filter_statistic.setStddevMulThresh (statistic_p2->value());                      //设置判断是否为离群点的阀值，用来倍乘标准差，也就是上面的std_mul
    filter_statistic.filter (*cloud_filtered);                    //滤波结果存储到cloud_filtered
    cloud_buffer.emplace_back(cloud_filtered);
    if(cloud_buffer.back()->size()==0)
    {
        cloud_buffer.pop_back();
        qDebug()<<"The PointCloud is empty, please double-check the parameters";
        return false;
    }
    if(cloud_buffer.back()->size()==cloud_buffer[cloud_buffer.size()-2]->size())
    {
        cloud_buffer.pop_back();
        qDebug()<<"The filter is useless, please double-check the parameters";
        return false;
    }
    qDebug()<<"The amount of statistic filtered PointCloud is :"<<cloud_buffer.back()->size();
    return true;
}

bool sg_point_cloud_postprocess::remove_manual()
{
    if(cloud_buffer.empty())
    {
        qDebug()<<"ERROR: NO POINTCLOUD DETECTED!";
        return false;
    }
    auto cloud = cloud_buffer.back();
    for(int i =0;i<cloud->points.size();i++)
    {
        if(cloud->points[i]._PointXYZRGB::x<0)
            indexs_selected.emplace_back(i);
    }
    for(int i = indexs_selected.size()-1;i>-1;i--)
    {
        cloud->erase(cloud->begin()+indexs_selected[i]);
    }
    return true;
}


