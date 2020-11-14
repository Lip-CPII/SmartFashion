#include "lp_draw_dots.h"


#include "lp_renderercam.h"

#include <QSizePolicy>
#include <QVBoxLayout>
#include <QGroupBox>
#include <QSlider>
#include <QGridLayout>
#include <QMouseEvent>
#include <QOpenGLFramebufferObject>
#include <QOpenGLContext>
#include <QOpenGLShaderProgram>
#include <QOpenGLExtraFunctions>
#include <QOpenGLTexture>
#include <QOpenGLBuffer>
#include <QLabel>
#include <QSpinBox>
#include <QMatrix4x4>
#include <QPushButton>
#include <QtConcurrent/QtConcurrent>

REGISTER_FUNCTIONAL_IMPLEMENT(LP_Draw_Dots, Knitting Instructions, menuTools)

LP_Draw_Dots::LP_Draw_Dots(QObject *parent) : LP_Functional(parent)
  ,mInitialized(false)
{
    qDebug() << mImage.load("../../Smart_Fashion/App/images/background.jpg");
}

LP_Draw_Dots::~LP_Draw_Dots()
{

}

QWidget *LP_Draw_Dots::DockUi()
{
    mWidget = std::make_shared<QWidget>();

    QVBoxLayout *layout = new QVBoxLayout(mWidget.get());

    QGroupBox *group = new QGroupBox(mWidget.get());

    QSlider *slider = new QSlider();
    slider->setRange(0, 16777215);
    slider->setValue(2856582);
    slider->setOrientation(Qt::Horizontal);
    mSlider = slider;

    QLabel *rLabel = new QLabel(tr("Red"));
    QLabel *gLabel = new QLabel(tr("Green"));
    QLabel *bLabel = new QLabel(tr("Blue"));

    mR = new QSpinBox(mWidget.get());
    mG = new QSpinBox(mWidget.get());
    mB = new QSpinBox(mWidget.get());

    mR->setRange(0,255);
    mG->setRange(0,255);
    mB->setRange(0,255);

    mR->setValue(255);

    QGridLayout *glayout = new QGridLayout;

    glayout->addWidget(rLabel, 0, 0);
    glayout->addWidget(gLabel, 0, 1);
    glayout->addWidget(bLabel, 0, 2);

    glayout->addWidget(mR, 1, 0);
    glayout->addWidget(mG, 1, 1);
    glayout->addWidget(mB, 1, 2);

    glayout->addWidget(slider, 2,0, 1, 3);

    group->setLayout(glayout);
    group->setMaximumHeight(100);
    layout->addWidget(group);


    QPushButton *button = new QPushButton(tr("Export"),mWidget.get());
    layout->addWidget(button);

    mWidget->setLayout(layout);

    connect(slider, &QSlider::valueChanged,
            this, &LP_Functional::glUpdateRequest);

    return mWidget.get();
}

bool LP_Draw_Dots::Run()
{
    emit glUpdateRequest();
    return false;
}

void LP_Draw_Dots::FunctionalRender(QOpenGLContext *ctx, QSurface *surf, QOpenGLFramebufferObject *fbo, const LP_RendererCam &cam, const QVariant &options)
{
    Q_UNUSED(cam)
    Q_UNUSED(surf)  //Mostly not used within a Functional.
    Q_UNUSED(options)   //Not used in this functional.


    if ( !mInitialized ){   //The OpenGL resources, e.g. Shader, not initilized
        initializeGL();     //Call the initialize member function.

        mImage = QImage(0.1*fbo->width(), 0.1*fbo->height(), QImage::Format_RGB888);
        mImage.fill(Qt::black);
        qDebug() << "FBO : " << fbo->width() << "x" << fbo->height();
    }                       //Not compulsory, (Using member function for cleaness only)


//    QMatrix4x4 proj = cam->ProjectionMatrix(),
//               view = cam->ViewMatrix();


    static auto rainbow = [](int p, int np, float&r, float&g, float&b) {    //16,777,216
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

    auto f = ctx->extraFunctions();         //Get the OpenGL functions container

    f->glEnable(GL_DEPTH_TEST);             //Enable depth test

    fbo->bind();
    mProgram->bind();                       //Bind the member shader to the context

    QOpenGLTexture tex_(mImage);
    tex_.setMinMagFilters(QOpenGLTexture::Nearest,QOpenGLTexture::Nearest);
    tex_.create();
    tex_.bind();

    mProgram->setUniformValue("u_tex", 0);
    mProgram->setUniformValue("v4_color", QVector4D(0.0f,0.0f,0.0f,1.0f));
    mProgram->setUniformValue("m4_mvp", QMatrix4x4() );

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
    tex_.destroy();

    mProgram->disableAttributeArray("a_tex");

    //Draw grid-lines
    f->glLineWidth(1.0f);

    const float hStep = 2.0f / mImage.width(),      //In Screen space
                vStep = 2.0f / mImage.height();

    float r, g, b;
    rainbow(mSlider->value(), mSlider->maximum(), r, g, b);
    mProgram->setUniformValue("v4_color", QVector4D(r, g, b, 0.4f));

    std::vector<QVector3D> l;
    for ( int i=0; i<=mImage.width(); ++i ){
        l.emplace_back(QVector3D( hStep*i - 1.0f, 1.0f, 0.1f ));
        l.emplace_back(QVector3D( hStep*i - 1.0f, -1.0f, 0.1f ));
    }
    for ( int j=0; j<=mImage.height(); ++j ){//Horizontal
        l.emplace_back(QVector3D( -1.0f, j*vStep - 1.0f, 0.1f ));
        l.emplace_back(QVector3D( 1.0f, j*vStep - 1.0f, 0.1f ));
    }

    f->glEnable(GL_BLEND);
    f->glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    mProgram->setAttributeArray("a_pos", l.data());
    f->glDrawArrays(GL_LINES, 0, GLsizei(l.size()));
    f->glDisable(GL_BLEND);


    mProgram->disableAttributeArray("a_pos"); //Disable the "a_pos" buffer

    mProgram->release();                        //Release the shader from the context
    fbo->release();
    f->glDisable(GL_DEPTH_TEST);
}

void LP_Draw_Dots::initializeGL()
{
    constexpr char vsh[] =
            "attribute vec3 a_pos;\n"       //The position of a point in 3D that used in FunctionRender()
            "attribute vec2 a_tex;\n"       //The uv-coord input
            "uniform mat4 m4_mvp;\n"
            "varying vec2 texCoord;\n"      //uv-coord to Fragment shader
            "void main(){\n"
            "   gl_Position = m4_mvp * vec4(a_pos, 1.0);\n"//Output the OpenGL position
            "   texCoord = a_tex;\n"
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

    //Assume full screen plane in normalized projection space [-1,1]x[-1,1]x[-1,1] with fixed depth
    std::vector<QVector3D> pos = {QVector3D(1.0f,-1.0f,0.9f),
                                      QVector3D(1.0f, 1.0f, 0.9f),
                                      QVector3D(-1.0f, -1.0f, 0.9f),
                                      QVector3D(-1.0f, 1.0f, 0.9f)};

    //The corresponding uv-coord
    std::vector<QVector2D> texCoord = {QVector2D(1.0f,1.0f),
                                      QVector2D(1.0f, 0.0f),
                                      QVector2D(0.0f, 1.0f),
                                      QVector2D(0.0f, 0.0f)};

    const size_t nVs = pos.size();

    QOpenGLBuffer *posBuf = new QOpenGLBuffer(QOpenGLBuffer::VertexBuffer);
    posBuf->setUsagePattern(QOpenGLBuffer::StreamDraw);
    posBuf->create();
    posBuf->bind();
    posBuf->allocate(int( nVs * ( sizeof(QVector2D) + sizeof(QVector3D))));
    //mVBO->allocate( m->points(), int( m->n_vertices() * sizeof(*m->points())));
    auto ptr = static_cast<float*>(posBuf->map(QOpenGLBuffer::WriteOnly));
    auto pptr = pos.begin();
    auto tptr = texCoord.begin();
    for ( size_t i=0; i<nVs; ++i, ++pptr, ++tptr ){
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
    auto iptr = static_cast<uint*>(indBuf->map(QOpenGLBuffer::WriteOnly));
    memcpy(iptr, indices.data(), indices.size() * sizeof(indices[0]));
    indBuf->unmap();
    indBuf->release();

    mIndices = indBuf;

    mInitialized = true;
}

bool LP_Draw_Dots::eventFilter(QObject *watched, QEvent *event)
{
    if ( QEvent::MouseButtonRelease == event->type()){
        auto e = static_cast<QMouseEvent*>(event);
        auto widget = qobject_cast<QWidget*>(watched);

        const float rw = float(mImage.width())/widget->width(),
                    rh = float(mImage.height())/widget->height();
        uint x = e->pos().x() * rw,
             y = e->pos().y() * rh;
        if ( e->button() == Qt::LeftButton ){
            mImage.setPixel(x, y, qRgb(mR->value(),mG->value(),mB->value()));
            emit glUpdateRequest();
            return true;
        }else if ( e->button() == Qt::RightButton){
            mImage.setPixel(x, y, qRgb(0, 0, 0));
            emit glUpdateRequest();
            return true;
        }
    }

    return QObject::eventFilter(watched, event);
}
