#include "lp_plugin_pifuhd.h"

#include "lp_openmesh.h"
#include "lp_renderercam.h"

#include "renderer/lp_glselector.h"

#include "Commands/lp_cmd_import_opmesh.h"
#include "Commands/lp_commandmanager.h"

#include <QLabel>
#include <QPushButton>
#include <QFileDialog>
#include <QMatrix4x4>
#include <QAction>
#include <QSizePolicy>
#include <QVBoxLayout>
#include <QMouseEvent>
#include <QOpenGLContext>
#include <QOpenGLShaderProgram>
#include <QOpenGLFramebufferObject>
#include <QOpenGLExtraFunctions>
#include <QtConcurrent/QtConcurrent>

LP_Plugin_PIFuHD::~LP_Plugin_PIFuHD()
{
    emit glContextRequest([this](){
        delete mProgram;
        mProgram = nullptr;
    });
    Q_ASSERT(!mProgram);
}

QWidget *LP_Plugin_PIFuHD::DockUi()
{
    mWidget = std::make_shared<QWidget>();
    auto widget = mWidget.get();
    QVBoxLayout *layout = new QVBoxLayout;

    QPushButton *button = new QPushButton("Select Images Directory",widget);
    QLabel *label = new QLabel(widget);
    label->setWordWrap(true);
    QPushButton *venv = new QPushButton("Select Virtual Env.",widget);


    layout->addWidget(button);
    layout->addWidget(label);
    layout->addWidget(venv);

    layout->addStretch();
    widget->setLayout(layout);


    connect(venv, &QPushButton::clicked,
            [label](){
        auto venvPath = QFileDialog::getExistingDirectory(0,"Select python virtual environment");
        if ( venvPath.isEmpty()){
            return;
        }
        label->setText(venvPath);
    });

    connect(button, &QPushButton::clicked,
            [this,label](){
        auto imagePath = QFileDialog::getExistingDirectory(0,"Select image directory");
        if ( imagePath.isEmpty()){
            return;
        }

#ifdef Q_OS_WIN
        auto future = QtConcurrent::run(&mPool,[imagePath,label](){
            QProcess proc;
            connect(&proc, &QProcess::readyReadStandardError,[&](){
               qDebug() << proc.readAllStandardError().data();
            });
            connect(&proc, &QProcess::readyReadStandardOutput,[&](){
               qDebug() <<  proc.readAllStandardOutput().data();
            });

            QDir imgDir(imagePath);
            imgDir.setNameFilters({"*.jpg","*.jpeg","*.png"});

            auto imglist = imgDir.entryList(QDir::Files | QDir::NoSymLinks);
            for ( auto &img : imglist ){
                QFileInfo info_(imgDir.path(), img);
                if (!QFile::copy(info_.absoluteFilePath(),
                                 QDir::currentPath()+"/plugins/lp_plugin_pifuhd/openpose_output/"+info_.fileName())){
                    qDebug() << "Invalid image : " << info_.absoluteFilePath();
                    continue;
                }
            }

            qDebug() << "Start OpenPose";
            QFileInfo info(imagePath);

            proc.setWorkingDirectory(QDir::currentPath()+"/plugins/lp_plugin_pifuhd/openpose");
            proc.start("plugins/lp_plugin_pifuhd/openpose/OpenPoseDemo.exe",
                       {"--image_dir",
                        QDir::currentPath()+"/plugins/lp_plugin_pifuhd/openpose_output",
                        "--write_json",
                        "../openpose_output",
                        "--display", "0",
                        "--render_pose","0"});
            if (!proc.waitForStarted(3000)){
                qDebug() << "Failed to start OpenPose";
                qDebug() << proc.readAllStandardError() << "\n"
                         << proc.readAllStandardOutput();
                return;
            }
            if ( !proc.waitForFinished(900000)){
                proc.kill();
                qDebug() << "Process to long. Forced to stop!";
            }
            qDebug() << "Finished OpenPose";
            qDebug() << "Start PIFu HD";
            proc.start("cmd",{});
            auto venvPath = label->text();
            if ( !venvPath.isEmpty()){
                proc.write(venvPath.toUtf8() + "/Scripts/activate\r\n");
            }
            proc.write("cd " + QDir::currentPath().toUtf8() + "/plugins/lp_plugin_pifuhd/PIFu/pifuhd\r\n");
            proc.write("python -m apps.simple_test -i ../../openpose_output -o ../../pifuhd_output\r\n ");

            proc.write("deactivate\r\n");
            proc.write("exit\r\n");
            if ( !proc.waitForFinished(900000)){
                proc.kill();
                qDebug() << "Process to long. Forced to stop!";
            }
            proc.kill();
            qDebug() << "Finished Reconstruction";
            qDebug() << "Emit Import Mesh";

            QDir reconPath(QDir::currentPath().toUtf8() + "/plugins/lp_plugin_pifuhd/pifuhd_output/pifuhd_final/recon");
            reconPath.setNameFilters({"*.obj"});
            auto meshlist = reconPath.entryList(QDir::Files | QDir::NoSymLinks);
            for ( auto &meshfile : meshlist ){
                auto cmd = new LP_Cmd_Import_OpMesh;
                cmd->SetFile( reconPath.path() + "/" + meshfile);
                if ( !cmd->VerifyInputs()){
                    delete cmd;
                    continue;
                }
                LP_CommandGroup::gCommandGroup->ActiveStack()->Push(cmd);   //Execute the command
            }
            qDebug() << "Process completed";
        });
#else
        label->setText("Operation is currently supported in Windows only.");
#endif
    });

    return widget;
}

bool LP_Plugin_PIFuHD::Run()
{
    mPool.setMaxThreadCount(std::max(1, mPool.maxThreadCount()-2));
    g_GLSelector->ClearSelected();
    return false;
}

void LP_Plugin_PIFuHD::FunctionalRender(QOpenGLContext *ctx, QSurface *surf, QOpenGLFramebufferObject *fbo, const LP_RendererCam &cam, const QVariant &options)
{
    Q_UNUSED(surf)
    Q_UNUSED(options)

    if ( !mInitialized ){
        initializeGL();
    }
}

void LP_Plugin_PIFuHD::initializeGL()
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

QString LP_Plugin_PIFuHD::MenuName()
{
    return tr("menuPlugins");
}

QAction *LP_Plugin_PIFuHD::Trigger()
{
    if ( !mAction ){
        mAction = new QAction("PIFu HD");
    }
    return mAction;
}
