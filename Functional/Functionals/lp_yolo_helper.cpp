#include "lp_yolo_helper.h"

#include "lp_renderercam.h"

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
#include <QCheckBox>
#include <QPainter>
#include <QPainterPath>
#include <QtConcurrent/QtConcurrent>

REGISTER_FUNCTIONAL_IMPLEMENT(LP_YOLO_Helper, YOLO Helper, menuTools)

LP_YOLO_Helper::LP_YOLO_Helper(QObject *parent) : LP_Functional(parent)
{
    mCurrentBoundingBox.first = QVector2D(std::numeric_limits<float>::max(),0.0f);
}

LP_YOLO_Helper::~LP_YOLO_Helper()
{
    if ( !mWatcher.isFinished()){
        mLock.lockForWrite();
        mStop = true;
        mLock.unlock();
        mWait.wakeOne();
        mWatcher.waitForFinished();
    }
}

QWidget *LP_YOLO_Helper::DockUi()
{
    mWidget = std::make_shared<QWidget>();

    QVBoxLayout *layout = new QVBoxLayout(mWidget.get());

    QCheckBox *checkbox = new QCheckBox(tr("View Gray Scale"),mWidget.get());
    layout->addWidget(checkbox);

    QPushButton *clearBB = new QPushButton(tr("Clear BoundingBoxes"));
    layout->addWidget(clearBB);

    QPushButton *import = new QPushButton(tr("Import Video"));
    layout->addWidget(import);

    QPushButton *importbx = new QPushButton(tr("Import BoundingBox"));
    layout->addWidget(importbx);

    QGroupBox *box = new QGroupBox;
    QGridLayout *_glayout = new QGridLayout;

    QPushButton *_export = new QPushButton(tr("Export YOLO set"));
    _glayout->addWidget(_export,0,0);

    QPushButton *_reset = new QPushButton(tr("Reset Export Path"));
    _glayout->addWidget(_reset,0,1);

    QPushButton *_litup = new QPushButton(tr("Lightup"));
    _glayout->addWidget(_litup,1,0);

    QPushButton *_spreadData = new QPushButton(tr("Split Train/Eval."));
    _glayout->addWidget(_spreadData,1,1);

    box->setLayout(_glayout);
    layout->addWidget(box);

    mLabel = new QLabel("");
    layout->addWidget(mLabel);

    mClasses = new QComboBox(mWidget.get());
    QStringList items{"0","1","2","3"};     //Classnames

    mClasses->addItems(items);
    layout->addWidget(mClasses);

    QGroupBox *group = new QGroupBox(mWidget.get());
    QGridLayout *glayout = new QGridLayout;

    QPushButton *play = new QPushButton(tr("Play"));
    QPushButton *forward = new QPushButton(tr(">> 15 frames"));
    QPushButton *backward = new QPushButton(tr("15 frames <<"));
    QPushButton *allback = new QPushButton(tr("|<"));
    QPushButton *end = new QPushButton(tr(">|"));
    QPushButton *autoExp = new QPushButton(tr("AutoExp"));

    glayout->addWidget(allback,0,0);
    glayout->addWidget(backward,1,0);
    glayout->addWidget(play,0,1);
    glayout->addWidget(forward,1,1);
    glayout->addWidget(end,0,2);
    glayout->addWidget(autoExp,1,2);

    group->setLayout(glayout);
    group->setMaximumHeight(100);
    group->setEnabled(false);

    layout->addWidget(group);

    mWidget->setLayout(layout);

    connect(checkbox, &QCheckBox::clicked,
            [this](const bool &checked){
        mbGreyScale = checked;
        emit glUpdateRequest();
    });

    connect(clearBB, &QPushButton::clicked,
            [this](){
        mLock.lockForWrite();
        mBoundingBoxesYOLO.clear();
        mLock.unlock();
        emit glUpdateRequest();
    });

    std::function<QImage(const cv::Mat &)> toQImage = [this](const cv::Mat &frame){
        if ( mbGreyScale ){
            cv::Mat gray;
            cv::cvtColor(frame, gray, cv::COLOR_BGR2GRAY);
            return QImage((uchar*) gray.data, int(gray.cols),
                            int(gray.rows), int(gray.step), QImage::Format_Grayscale8).copy();
        }
        return QImage((uchar*) frame.data, int(frame.cols),
                        int(frame.rows), int(frame.step), QImage::Format_BGR888).copy();
    };

    connect(import, &QPushButton::clicked,
            [this, group, toQImage](){
        auto fileName = QFileDialog::getOpenFileName(mWidget.get(),tr("Open Video"),QString(),
                                                     "*.mp4 *.flv *.avi");
        if ( fileName.isEmpty()){
            return;
        }
        mVideoFile = fileName;
        if ( !mCVCam ){
            mCVCam = std::make_shared<cv::VideoCapture>();
        }

        bool rc = mCVCam->open(mVideoFile.toStdString().c_str());
        Q_ASSERT(rc);
        auto fps = mCVCam->get(cv::CAP_PROP_FPS);
        auto framecount = uint(mCVCam->get(cv::CAP_PROP_FRAME_COUNT));
        mTotalTime = 1000.0*framecount / fps;
        qDebug() << mTotalTime;
        cv::Mat frame;

        *mCVCam >> frame;

        mImage = toQImage(frame);

        emit glUpdateRequest();
        mCurrentTime = 0.0;
        group->setEnabled(true);
    });

    connect(importbx, &QPushButton::clicked,
            [this, toQImage](){
        if ( mImage.isNull()){
            QMessageBox::information(0,"Information", "Import a reference image first");
            return;
        }
        auto fileName = QFileDialog::getOpenFileName(mWidget.get(),tr("Open Bounding boxes"),QString(),
                                                     "*.txt");
        if ( fileName.isEmpty()){
            return;
        }
        QFile file(fileName);
        if ( !file.open(QIODevice::ReadOnly)){
            return;
        }
        auto cam = mCam.lock();
        auto mvp = cam->ViewportMatrix() * cam->ProjectionMatrix() * cam->ViewMatrix();
        mvp = mvp.inverted();

        const auto imgW = mImage.width(),
                   imgH = mImage.height();
        const auto hImgW = 0.5f*imgW,
                   hImgH = 0.5f*imgH;

        QTextStream in(&file);
        QString line;
        while (in.readLineInto(&line)) {
            auto items = line.split(" ");
            YOLO_BoundingBox box;
            box.mClass = items.first().toInt();
            QVector3D center(items.at(1).toFloat() * imgW, items.at(2).toFloat() * imgH, 0.0f);
            float w = items.at(3).toFloat() * imgW,
                  h = items.at(4).toFloat() * imgH;

            box.mPickPoints3D.first = QVector3D(center.x() - 0.5f * w - hImgW,
                                                hImgH - center.y() + 0.5f*h, 0.1f);
            box.mPickPoints3D.second = QVector3D(center.x() + 0.5f * w - hImgW,
                                                 hImgH - center.y() - 0.5f*h, 0.1f);

            box.mPickPoints.first = QVector2D(center.x() - 0.5f * w - hImgW,
                                              hImgH - center.y() + 0.5f*h);
            box.mPickPoints.second = QVector2D(center.x() + 0.5f * w - hImgW,
                                               hImgH - center.y() - 0.5f*h);

            mBoundingBoxesYOLO.emplace_back(std::move(box));

            qDebug() << items;
        }
        file.close();
        emit glUpdateRequest();
    });


    connect(_export, &QPushButton::clicked,
            this, &LP_YOLO_Helper::exportYOLOset);

    connect(_reset, &QPushButton::clicked,
            [this](){
        mExportPath.clear();
    });

    connect(_litup, &QPushButton::clicked,
            [this](){
        if ( mVideoFile.isEmpty()){
            return;
        }
        qDebug() << mVideoFile;
        QProcess proc;
        connect(&proc, &QProcess::readyReadStandardError,[&](){
           qDebug() << proc.readAllStandardError();
           proc.write("y \r\n");
        });
        connect(&proc, &QProcess::readyReadStandardOutput,[&](){
           qDebug() << proc.readAllStandardOutput();
        });
        QFileInfo info(mVideoFile);
        proc.start("ffmpeg",{"-i", mVideoFile.toUtf8(), "-vf", "eq=brightness=0.06:saturation=2", "-c:a",
                             "copy", info.absolutePath()+"/litup_"+info.fileName()});
        if (!proc.waitForStarted(3000)){
            return;
        }
        proc.waitForFinished(900000);
    });

    connect(_spreadData, &QPushButton::clicked,
            [this](){
        auto folders = QFileDialog::getExistingDirectory(0,"Select folder");
        if ( folders.isEmpty()){
            return;
        }
        QDir folders_dir(folders);
        if ( !folders_dir.exists("images") || !folders_dir.exists("labels")){
            qDebug() << "No image/labels folder";
            return;
        }

        QDir newFolders(folders);
        auto tmp = tr("split_data_%1").arg(QDateTime::currentSecsSinceEpoch());
        if ( !newFolders.mkdir(tmp)){
            qDebug() << "Cannot create split data folder : " << newFolders.path()+"/"+tmp;
            return;
        }
        newFolders.cd(tmp);


        QDir imageFolders(folders_dir);
        imageFolders.cd("images");
        auto subFolders = imageFolders.entryList(QDir::Dirs | QDir::NoSymLinks | QDir::NoDotAndDotDot);
        qDebug() << subFolders;

        uint nfiles = 0;
        const uint nStep = 5;   //Every nStep files becomes the eval.

        const auto newTrainImagePath = newFolders.path() + "/train/images/";
        const auto newTrainLabelsPath = newFolders.path() + "/train/labels/";
        const auto newEvalImagePath = newFolders.path() + "/eval/images/";
        const auto newEvalLabelsPath = newFolders.path() + "/eval/labels/";

        for ( auto &sub : subFolders ){
            const auto subImagePath = imageFolders.path() + "/" + sub + "/";
            const auto subLabelPath = folders_dir.path() + "/labels/" + sub + "/";
            QDir sub_dir(subImagePath);
            auto files = sub_dir.entryList(QDir::Files | QDir::NoSymLinks, QDir::Time);

            newFolders.mkpath("train/images/"+sub);
            newFolders.mkpath("train/labels/"+sub);
            newFolders.mkpath("eval/images/"+sub);
            newFolders.mkpath("eval/labels/"+sub);

            const auto newSubTrainImagePath = newTrainImagePath + sub + "/";
            const auto newSubTrainLabelsPath = newTrainLabelsPath + sub + "/";
            const auto newSubEvalImagePath = newEvalImagePath + sub + "/";
            const auto newSubEvalLabelsPath = newEvalLabelsPath + sub + "/";

            for ( auto &file : files ){
                QFileInfo info(file);
                if ( 0 == nfiles % nStep ){//Copy to eval
                    if ( !QFile::copy(subImagePath + file,
                                      newSubEvalImagePath + file)){
                        qDebug() << "Copy image failed : " << subImagePath + file + " -> " +
                                    newSubEvalImagePath + file;
                    }
                    if ( !QFile::copy(subLabelPath +
                                      info.baseName() + ".txt",
                                      newSubEvalLabelsPath + info.baseName() + ".txt")){
                        qDebug() << "Copy label failed : " << folders_dir.path() + "/labels/" + sub + "/" +
                                    info.baseName() + ".txt" + " -> " +
                                    newSubEvalLabelsPath + info.baseName() + ".txt";
                    }
                }else{  //Copy to train
                    if ( !QFile::copy(subImagePath + file,
                                      newSubTrainImagePath + file)){
                        qDebug() << "Copy image failed : " << subImagePath + file + " -> " +
                                    newSubEvalImagePath + file;
                    }
                    if ( !QFile::copy(subLabelPath +
                                      info.baseName() + ".txt",
                                      newSubTrainLabelsPath + info.baseName() + ".txt")){
                        qDebug() << "Copy label failed : " << folders_dir.path() + "/labels/" + sub + "/" +
                                    info.baseName() + ".txt" + " -> " +
                                    newSubEvalLabelsPath + info.baseName() + ".txt";
                    }
                }
                ++nfiles;
            }
        }

        qDebug() << "Total processed files : " << nfiles;

        return;
    });

    std::function<void(const int &)> jumpFrame = [this, toQImage](const int &jumpFrames){
        if ( !mCVCam ){
            return;
        }
        const double frame_rate = mCVCam->get(cv::CAP_PROP_FPS);
        // Calculate number of msec per frame.
        // (msec/sec / frames/sec = msec/frame)
        const double frame_msec = 1000.0 / frame_rate;

        mLock.lockForWrite();

        if ( std::numeric_limits<int>::max() == jumpFrames){
            auto framecount = uint(mCVCam->get(cv::CAP_PROP_FRAME_COUNT));
            mCurrentTime = (framecount - 1) * frame_msec;
        }else if (-std::numeric_limits<int>::max() == jumpFrames){
            mCurrentTime = 0.0;
        }
        else{
            mCurrentTime += jumpFrames * frame_msec;
        }
        if ( mCurrentTime < 0.0 ){
            mCurrentTime = 0.0;
            QMessageBox::information(mWidget.get(), tr("Information"), tr("At beginning"));
        }
        else if ( mCurrentTime > mTotalTime ){
            auto framecount = uint(mCVCam->get(cv::CAP_PROP_FRAME_COUNT));
            mCurrentTime = (framecount - 1) * frame_msec;
            QMessageBox::information(mWidget.get(), tr("Information"), tr("At end"));
        }
        mCVCam->set(cv::CAP_PROP_POS_MSEC, mCurrentTime);
        cv::Mat frame;
        *mCVCam >> frame;

        mImage = toQImage(frame);

        mLock.unlock();

        emit glUpdateRequest();
    };
    connect(play, &QPushButton::clicked,
            [this, toQImage](){
        if ( mWatcher.isRunning()){
            mLock.lockForWrite();
            if ( mPause ){
                mWait.wakeOne();
                mPause = false;
            }else{
                mPause = true;
            }
            mLock.unlock();
            return;
        }
        auto future = QtConcurrent::run(&mPool,[this,toQImage](){
            if ( !mCVCam ){
                return;
            }
            const double frame_rate = mCVCam->get(cv::CAP_PROP_FPS);
            const double frame_msec = 1000.0 / frame_rate;
            cv::Mat frame;
            mLock.lockForRead();
            mCVCam->read(frame);
            mLock.unlock();
            while (!frame.empty()){
                mLock.lockForWrite();
                if ( mPause ){
                    mWait.wait(&mLock);
                }
                if ( mStop ){
                    mLock.unlock();
                    break;
                }
                mImage = toQImage(frame);
                mCurrentTime += frame_msec;

                *mCVCam >> frame;
                mLock.unlock();
                QThread::msleep(5);
                emit glUpdateRequest();
            }
        });
        mWatcher.setFuture(future);
    });

    connect(autoExp, &QPushButton::clicked,
            [this, toQImage](){
        if ( mWatcher.isRunning()){
            mLock.lockForWrite();
            if ( mPause ){
                mWait.wakeOne();
                mPause = false;
            }else{
                mPause = true;
            }
            mLock.unlock();
            return;
        }
        auto future = QtConcurrent::run(&mPool,[this,toQImage](){
            if ( !mCVCam ){
                return;
            }
            const double frame_rate = mCVCam->get(cv::CAP_PROP_FPS);
            const double frame_msec = 1000.0 / frame_rate;
            cv::Mat frame;
            mLock.lockForRead();
            mCVCam->read(frame);
            mLock.unlock();

            const int skipFrames = 15;
            while (!frame.empty()){
                mLock.lockForWrite();
                if ( mPause ){
                    mWait.wait(&mLock);
                }
                if ( mStop ){
                    mLock.unlock();
                    break;
                }
                mImage = toQImage(frame);
                mCurrentTime += frame_msec;
                for ( auto i=0; i<skipFrames; ++i ){//Skip frames
                    mCVCam->grab();
                    mCurrentTime += frame_msec;
                }
                *mCVCam >> frame;
                mLock.unlock();

                exportYOLOset();    //Export the set

                QThread::usleep(1);
                emit glUpdateRequest();
            }
        });
        mWatcher.setFuture(future);
    });

    connect(forward, &QPushButton::clicked,
            this,std::bind(jumpFrame,15));
    connect(backward, &QPushButton::clicked,
            this,std::bind(jumpFrame, -15));
    connect(allback, &QPushButton::clicked,
            this,std::bind(jumpFrame, -std::numeric_limits<int>::max()));
    connect(end, &QPushButton::clicked,
            this,std::bind(jumpFrame, std::numeric_limits<int>::max()));
    return mWidget.get();
}

bool LP_YOLO_Helper::Run()
{
    mPool.setMaxThreadCount(std::max(1, mPool.maxThreadCount()-2));
    return false;
}

void LP_YOLO_Helper::FunctionalRender(QOpenGLContext *ctx, QSurface *surf, QOpenGLFramebufferObject *fbo, const LP_RendererCam &cam, const QVariant &options)
{
    Q_UNUSED(surf)  //Mostly not used within a Functional.
    Q_UNUSED(options)   //Not used in this functional.

    if ( mImage.isNull()){
        return;
    }

    if ( !mInitialized ){   //The OpenGL resources, e.g. Shader, not initilized
        initializeGL();     //Call the initialize member function.
    }                       //Not compulsory, (Using member function for cleaness only)

    if ( !mCam.lock()){
        mCam = cam;
    }

    if ( mFBOHeight != cam->ResolutionY()){
        cam->setDiagonal(fbo->height());
        cam->SetPerspective(false);
        cam->RefreshCamera();
        mFBOHeight = cam->ResolutionY();
        qDebug() << mImage.size();
        qDebug() << "FBO: " << fbo->size();
    }

    QMatrix4x4 proj = cam->ProjectionMatrix(),
               view = cam->ViewMatrix(),
               vp   = cam->ViewportMatrix();


    auto f = ctx->extraFunctions();         //Get the OpenGL functions container

    f->glEnable(GL_DEPTH_TEST);             //Enable depth test

    fbo->bind();
    mProgram->bind();                       //Bind the member shader to the context

    mLock.lockForRead();

    QOpenGLTexture tex_(mImage);
    tex_.setMinMagFilters(QOpenGLTexture::Nearest,QOpenGLTexture::Nearest);
    tex_.create();

    mLock.unlock();

    tex_.bind();

    mProgram->setUniformValue("u_tex", 0);
    mProgram->setUniformValue("m4_mvp", proj * view );
    mProgram->setUniformValue("v4_color", QVector4D(0.0f,0.0f,0.0f,1.0f));

    mProgram->enableAttributeArray("a_pos");
    mProgram->enableAttributeArray("a_tex");

    mVBO->bind();
    mIndices->bind();

    mProgram->setAttributeBuffer("a_pos",GL_FLOAT, 0, 3, 20);
    mProgram->setAttributeBuffer("a_tex",GL_FLOAT, 12, 2, 20);

    f->glDrawElements(  GL_TRIANGLE_STRIP,
                        4,
                        GL_UNSIGNED_INT,
                        0);//Actually draw all the image

    mVBO->release();
    mIndices->release();
    tex_.release();

    mProgram->disableAttributeArray("a_tex");

    //Draw bounding boxes
    auto mvp = vp * proj * view;
    mvp = mvp.inverted();
    std::function<std::vector<QVector3D>(const QVector3D&, const QVector3D &, bool)> getQuad =
            [&mvp](const QVector3D& a, const QVector3D &b, bool bTransform ){
        float minX = a.x(), minY = a.y(),
              maxX = b.x(), maxY = b.y();
        if ( minX > maxX ){
            std::swap(minX, maxX);
        }
        if ( minY > maxY ){
            std::swap(minY, maxY);
        }

        std::vector<QVector3D> box = {
            {minX, minY, 0.1f}, //Top-Left
            {minX, maxY, 0.1f}, //Bottom-Left
            {maxX, maxY, 0.1f},  //Bottom-Right
            {maxX, minY, 0.1f} //Top-Right
        };
        if ( bTransform ){
            std::transform(box.begin(), box.end(), box.begin()
                           ,[&mvp](const QVector3D& v){ return mvp*v;});
        }
        box.emplace_back(box.front());  //Close the loop
        return box;
    };

    static std::function<void(int,int,float&,float&,float&)> rainbow = [](int p, int np, float&r, float&g, float&b) {    //16,777,216
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

    f->glEnable(GL_BLEND);
    //Draw the current editing bounding box

    if ( mCurrentBoundingBox.first.x() != std::numeric_limits<float>::max() &&
         mCurrentBoundingBox.second.x() != std::numeric_limits<float>::max()){
        auto &&box = getQuad(mCurrentBoundingBox.first,
                             mCurrentBoundingBox.second, true);
        f->glLineWidth(3.0f);
        f->glEnable(GL_PROGRAM_POINT_SIZE);
        float r,g,b;
        rainbow(mClasses->currentIndex(), mClasses->count(), r,g,b);

        mProgram->setUniformValue("v4_color", QVector4D(r, g, b,0.6f));
        mProgram->setAttributeArray("a_pos", box.data());

        f->glDrawArrays(GL_LINE_STRIP, 0, 5);
        f->glDrawArrays(GL_POINTS, 0, 4);


        f->glDisable(GL_PROGRAM_POINT_SIZE);
    }

    //Draw the confirmed bounding boxes
    f->glLineWidth(1.0f);

    for ( auto &_b : mBoundingBoxesYOLO ){
        auto &&box = getQuad(_b.mPickPoints3D.first, _b.mPickPoints3D.second, false);
        mProgram->setAttributeArray("a_pos", box.data());
        float r,g,b;
        rainbow(_b.mClass, mClasses->count(), r,g,b);
        mProgram->setUniformValue("v4_color", QVector4D(r, g, b, 0.8f));
        f->glDrawArrays(GL_LINE_STRIP, 0, 5);
    }

    f->glDisable(GL_BLEND);

    mProgram->release();
    fbo->release();
}

void LP_YOLO_Helper::exportYOLOset()
{
    if ( mVideoFile.isEmpty() || mBoundingBoxesYOLO.empty() ||
         mImage.isNull()){
        return;
    }
    static std::function<QString(const int)> gen_random = [](const int len) {
        QString tmp_s;
        tmp_s.resize(len);
        constexpr char alphanum[] =
            "0123456789"
            "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
            "abcdefghijklmnopqrstuvwxyz";

        auto prand = QRandomGenerator::global();
        for (int i = 0; i < len; ++i)
            tmp_s[i] = alphanum[prand->generate() % (sizeof(alphanum) - 1)];

        return tmp_s;
    };

    qDebug() << "Check path";

    if ( mExportPath.isEmpty()){
        mExportPath = QFileDialog::getExistingDirectory(nullptr, tr("Export"));
        if ( mExportPath.isEmpty()){
            QMessageBox::information(mWidget.get(),tr("Information"),tr("File not exported"));
            return;
        }
    }

    QDir dir(mExportPath);
    if ( !dir.exists()/* && !dir.mkdir(dir_)*/){
        QMessageBox::information(mWidget.get(),tr("Information"),tr("Cannot create folder"));
        return;
    }
//    dir.cd(dir_);

    auto id = gen_random(16);
    QFile file(dir.absolutePath()+QString("/%1.txt").arg(id));

    if ( !file.open(QIODevice::WriteOnly)){
        QMessageBox::information(mWidget.get(),tr("Information"),tr("Cannot save file : %1").arg(file.fileName()));
        return;
    }

    qDebug() << "Export information";

    const float hImgW = 0.5f * mImage.width(),
                hImgH = 0.5f * mImage.height();
    const float invImgW = 1.0/mImage.width(),
                invImgH = 1.0/mImage.height();

    const QRectF imgRect(mImage.rect());
    //qDebug() << imgRect;
    QString chunk;
    for (auto &b : mBoundingBoxesYOLO){
        auto minx = std::min( b.mPickPoints3D.first.x(), b.mPickPoints3D.second.x()),
             miny = std::max( b.mPickPoints3D.first.y(), b.mPickPoints3D.second.y());   //Y-axis in 3D is up (Top is max)
        minx += hImgW;  //Change back to image-space [0, image.width()]
        miny = hImgH - miny;

        auto w  = std::abs(b.mPickPoints3D.first.x() - b.mPickPoints3D.second.x()),
             h  = std::abs(b.mPickPoints3D.first.y() - b.mPickPoints3D.second.y());

        QRectF rect(minx,miny,w,h);

        auto &&iRect = imgRect.intersected(rect);   //Clip within the image
        //qDebug() << rect << " " << iRect;

        if ( !iRect.isValid()){
            continue;
        }

        const auto &&c = iRect.center();
        w = invImgW * iRect.width();
        h = invImgH * iRect.height();

        chunk += QString("%1 %2 %3 %4 %5\n").arg(b.mClass)
                                        .arg(c.x() * invImgW).arg(c.y() * invImgH)
                                        .arg(w).arg(h);
    }
    chunk.resize(chunk.size()-1);
    QTextStream out(&file);
    out << chunk;
    file.close();
    mImage.save(dir.absolutePath()+QString("/%1.jpg").arg(id), "JPG", 100);

    mLabel->setText(tr("Exported %1").arg(id));
}

void LP_YOLO_Helper::initializeGL()
{
    constexpr char vsh[] =
            "attribute vec3 a_pos;\n"       //The position of a point in 3D that used in FunctionRender()
            "attribute vec2 a_tex;\n"       //The uv-coord input
            "uniform mat4 m4_mvp;\n"
            "varying vec2 texCoord;\n"      //uv-coord to Fragment shader
            "void main(){\n"
            "   gl_Position = m4_mvp * vec4(a_pos, 1.0);\n"//Output the OpenGL position
            "   texCoord = a_tex;\n"
            "   gl_PointSize = 10.0;\n"
            "}";

    constexpr char fsh[] =
            "uniform sampler2D u_tex;\n"    //The image texture
            "uniform vec4 v4_color;\n"      //The addition color for blending
            "varying vec2 texCoord;\n"
            "void main(){\n"
            "   vec3 color = v4_color.rgb + texture2D(u_tex, texCoord).rgb;\n"
            "   gl_FragColor = vec4(color, v4_color.a);\n" //Output the fragment color;
            "}";

    auto prog = new QOpenGLShaderProgram;   //Intialize the Shader with the above GLSL codes
    prog->addShaderFromSourceCode(QOpenGLShader::Vertex,vsh);
    prog->addShaderFromSourceCode(QOpenGLShader::Fragment,fsh);
    if (!prog->create() || !prog->link()){  //Check whether the GLSL codes are valid
        qDebug() << prog->log();
        return;
    }

    mProgram = prog;            //If everything is fine, assign to the member variable


    const auto w = mImage.width(),
               h = mImage.height();

    std::vector<QVector3D> pos = {QVector3D( 0.5f*w, -0.5*h, 0.0f),
                                      QVector3D( 0.5*w, 0.5*h, 0.0f),
                                      QVector3D(-0.5f*w, -0.5*h, 0.0f),
                                      QVector3D(-0.5f*w, 0.5*h, 0.0f)};

    //The corresponding uv-coord
    std::vector<QVector2D> texCoord = {QVector2D(1.0f,1.0f),
                                      QVector2D(1.0f, 0.0f),
                                      QVector2D(0.0f, 1.0f),
                                      QVector2D(0.0f, 0.0f)};

    const int nVs = int(pos.size());

    QOpenGLBuffer *posBuf = new QOpenGLBuffer(QOpenGLBuffer::VertexBuffer);
    posBuf->setUsagePattern(QOpenGLBuffer::StreamDraw);
    posBuf->create();
    posBuf->bind();
    posBuf->allocate(int( nVs * ( sizeof(QVector2D) + sizeof(QVector3D))));
    //mVBO->allocate( m->points(), int( m->n_vertices() * sizeof(*m->points())));
    auto ptr = static_cast<float*>(posBuf->mapRange(0,
                                                    int( nVs * ( sizeof(QVector2D) + sizeof(QVector3D))),
                                                    QOpenGLBuffer::RangeWrite | QOpenGLBuffer::RangeInvalidateBuffer));
    auto pptr = pos.begin();
    auto tptr = texCoord.begin();
    for ( int i=0; i<nVs; ++i, ++pptr, ++tptr ){
        memcpy(ptr, &(*pptr)[0], sizeof(QVector3D));
        memcpy(ptr+3, &(*tptr)[0], sizeof(QVector2D));
        ptr = std::next(ptr, 5);
        //qDebug() << QString("%1, %2, %3").arg((*nptr)[0]).arg((*nptr)[1]).arg((*nptr)[2]);
    }
    posBuf->unmap();
    posBuf->release();

    mVBO = posBuf;

    const std::vector<uint> indices = {0, 1, 2, 3};

    QOpenGLBuffer *indBuf = new QOpenGLBuffer(QOpenGLBuffer::IndexBuffer);
    indBuf->setUsagePattern(QOpenGLBuffer::StaticDraw);

    indBuf->create();
    indBuf->bind();
    indBuf->allocate(int( indices.size() * sizeof(indices[0])));
    auto iptr = static_cast<uint*>(indBuf->mapRange(0,
                                                    int( indices.size() * sizeof(indices[0])),
                                                    QOpenGLBuffer::RangeWrite | QOpenGLBuffer::RangeInvalidateBuffer));
    memcpy(iptr, indices.data(), indices.size() * sizeof(indices[0]));
    indBuf->unmap();
    indBuf->release();

    mIndices = indBuf;

    mInitialized = true;
}


bool LP_YOLO_Helper::eventFilter(QObject *watched, QEvent *event)
{
    auto widget = qobject_cast<QWidget*>(watched);
    const auto &&h = widget->height();
    mGLWidget = widget;

    mLabel->setText( mExportPath );

    if ( QEvent::MouseMove == event->type()){
        auto e = static_cast<QMouseEvent*>(event);
        if ( e->buttons() == Qt::LeftButton ){
            mCurrentBoundingBox.second = QVector2D(e->pos().x(), h - e->pos().y());
            emit glUpdateRequest();
            return true;
        }else if ((Qt::MiddleButton | Qt::RightButton) & e->buttons() &&
                  Qt::ControlModifier != e->modifiers()){
            return true;
        }
    }else if ( QEvent::MouseButtonPress == event->type()){
        auto e = static_cast<QMouseEvent*>(event);
        if ( e->buttons() == Qt::LeftButton ){
            mCurrentBoundingBox.first = QVector2D(e->pos().x(), h - e->pos().y());
            mCurrentBoundingBox.second.setX(std::numeric_limits<float>::max());
            emit glUpdateRequest();
            return true;
        }
    }else if ( QEvent::MouseButtonRelease == event->type()){
        auto e = static_cast<QMouseEvent*>(event);
        if ( e->button() == Qt::LeftButton ){
            bool bHit = false;
            auto cend = mBoundingBoxesYOLO.cend();
            for ( auto it = mBoundingBoxesYOLO.begin(); it != cend; ++it ){
                bHit = YOLO_BoundingBox::hit(e->pos(), *it, mCam);
                if ( bHit ) {
                    auto cam = mCam.lock();

                    QMatrix4x4 proj = cam->ProjectionMatrix(),
                               view = cam->ViewMatrix(),
                               vp   = cam->ViewportMatrix();
                    auto mvp = vp * proj * view;
                    auto p0 = it->mPickPoints3D.first,
                         p1 = it->mPickPoints3D.second;

                    mCurrentBoundingBox.first =  QVector2D(mvp * p0);
                    mCurrentBoundingBox.second = QVector2D(mvp * p1);
                    mClasses->setCurrentIndex(it->mClass);

                    mBoundingBoxesYOLO.erase(it);
                    break;
                }
            }
            if (!bHit){
                mCurrentBoundingBox.second = QVector2D(e->pos().x(), h - e->pos().y());
            }
            emit glUpdateRequest();
            return true;
        }else if ( e->button() == Qt::RightButton ){
            const auto &a = mCurrentBoundingBox.first,
                       &b = mCurrentBoundingBox.second;
            if ( a.x() != std::numeric_limits<float>::max() &&
                 b.x() != std::numeric_limits<float>::max()){
                //Check area

                float minX = a.x(), minY = a.y(),
                      maxX = b.x(), maxY = b.y();
                if ( minX > maxX ){
                    std::swap(minX, maxX);
                }
                if ( minY > maxY ){
                    std::swap(minY, maxY);
                }

                QRectF box(QPointF(minX,minY),QPointF(maxX,maxY));
                if ( box.width() < 10 || box.height() < 10 ){  //If the box is smaller than 100 pixel-square
                    QMessageBox::warning(mWidget.get(),tr("Warning"),
                                         tr("Bounding box too small/narrow (%1 x %2) < 100").arg(box.width())
                                         .arg(box.height()));
                    return QObject::eventFilter(watched, event);
                }
                //Transform it into the 3D coordinates
                auto cam = mCam.lock();

                QMatrix4x4 proj = cam->ProjectionMatrix(),
                           view = cam->ViewMatrix(),
                           vp   = cam->ViewportMatrix();
                auto mvp = vp * proj * view;
                mvp = mvp.inverted();

                auto p0 = mvp * QVector3D(mCurrentBoundingBox.first);
                auto p1 = mvp * QVector3D(mCurrentBoundingBox.second);

                YOLO_BoundingBox ybb;
                ybb.mClass = mClasses->currentIndex();
                ybb.mPickPoints = mCurrentBoundingBox;
                ybb.mPickPoints3D = {p0, p1};
                mBoundingBoxesYOLO.emplace_back(std::move(ybb));

                mCurrentBoundingBox.first = QVector2D(std::numeric_limits<float>::max(),0.0f);
                emit glUpdateRequest();
                return true;
            }
        }
    } else if ( QEvent::KeyPress == event->type()){
        auto e = static_cast<QKeyEvent*>(event);
        auto key = e->key();
        if ( key > 47 && key < 58){
            mClasses->setCurrentIndex(std::min(mClasses->count() - 1, key - 48));
            emit glUpdateRequest();
            return true;
        }
    }
    return QObject::eventFilter(watched, event);
}


void LP_YOLO_Helper::PainterDraw(QWidget *glW)
{
    if ( "openGLWidget_2" == glW->objectName()){
        return;
    }
    static std::function<void(int,int,float&,float&,float&)> rainbow = [](int p, int np, float&r, float&g, float&b) {    //16,777,216
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

    float h = glW->height();
    QPainter painter(glW);
    QPainterPath path;
    QFont orgFont = painter.font();
    int fontSize(13);
    QFont font("Arial", fontSize);
    QFontMetrics fmetric(font);
    const auto hOffset = fmetric.height()*0.8;
    QString text(mClasses->currentText());

    painter.setFont(font);
    auto cam = mCam.lock();
    if ( cam ){
        auto proj = cam->ProjectionMatrix(),
             view = cam->ViewMatrix(),
             vp   = cam->ViewportMatrix();

        auto mvp = vp * proj * view;

        for ( auto &b_ : mBoundingBoxesYOLO ){
            float x = b_.mPickPoints3D.first.x(),
                  y = b_.mPickPoints3D.first.y();
            if ( x > b_.mPickPoints3D.second.x()){
                x = b_.mPickPoints3D.second.x();
            }
            if ( y < b_.mPickPoints3D.second.y()){
                y = b_.mPickPoints3D.second.y();
            }
            QVector3D pos(x, y, 0.0);
            pos = mvp * pos;            //Transform back to OpenGL's screen space
            text = QString("%1.").arg(b_.mClass,3);
            float r,g,b;
            rainbow(b_.mClass, mClasses->count(), r, g, b );

            auto bR = fmetric.boundingRect(text);
            bR.translate(pos.x()+1, h-pos.y()+hOffset);
            painter.fillRect(bR,QBrush(qRgb(r*100,g*100,b*100)));
            painter.setPen(qRgb(r*255,g*255,b*255));
            painter.drawText(QPointF(pos.x(),
                                     h-pos.y()+hOffset), text );
        }
    }


    //Get top left pos
    if ( mCurrentBoundingBox.first.x() != std::numeric_limits<float>::max()){
        float x = mCurrentBoundingBox.first.x(),
                   y = mCurrentBoundingBox.first.y();
        if ( x > mCurrentBoundingBox.second.x()){
            x = mCurrentBoundingBox.second.x();
        }
        if ( y < mCurrentBoundingBox.second.y()){
            y = mCurrentBoundingBox.second.y();
        }
        text = mClasses->currentText();
        painter.setPen(mClasses->currentIndex() + Qt::red );
        painter.drawText(QPointF(x,
                                 h-y), text );
    }


    painter.setFont(orgFont);
}

bool LP_YOLO_Helper::YOLO_BoundingBox::hit(const QPoint &pos, const YOLO_BoundingBox &b_, std::weak_ptr<LP_RendererCamImpl> camw)
{
    auto cam = camw.lock();
    auto proj = cam->ProjectionMatrix(),
         view = cam->ViewMatrix(),
         vp   = cam->ViewportMatrix();
    auto mvp = vp * proj * view;

    const auto &&h = cam->ResolutionY();

    float x = b_.mPickPoints3D.first.x(),
          y = b_.mPickPoints3D.first.y();
    if ( x > b_.mPickPoints3D.second.x()){
        x = b_.mPickPoints3D.second.x();
    }
    if ( y < b_.mPickPoints3D.second.y()){
        y = b_.mPickPoints3D.second.y();
    }
    int fontSize(13);
    QFont font("Arial", fontSize);
    QFontMetrics fmetric(font);
    const auto hOffset = fmetric.height()*0.8;

    QVector3D org(x, y, 0.0);
    org = mvp * org;            //Transform back to OpenGL's screen space

    org.setY( h - org.y() + hOffset);

    auto text = QString("%1.").arg(b_.mClass,3);

    auto bR = fmetric.boundingRect(text);
    bR.translate(org.x(), org.y());

    qDebug() << bR << pos;
    return bR.contains(pos);
}
