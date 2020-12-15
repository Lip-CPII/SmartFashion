#include "lp_renderercam.h"
#include <QtMath>
#include <cstring>

LP_RendererCamImpl::LP_RendererCamImpl(LP_Object parent) : LP_ObjectImpl(parent)
{
    initialize();
}

bool LP_RendererCamImpl::IsPerspective() const
{
    return m_bPerspective;
}

void LP_RendererCamImpl::SetPerspective(bool bPerspective)
{
    m_bPerspective = bPerspective;
}

float LP_RendererCamImpl::FOV() const
{
    return m_FOV;
}

void LP_RendererCamImpl::SetFOV(float FOV)
{
    m_FOV = FOV;
}

float LP_RendererCamImpl::Left() const
{
    return m_Left;
}

void LP_RendererCamImpl::SetLeft(float Left)
{
    m_Left = Left;
}

float LP_RendererCamImpl::Right() const
{
    return m_Right;
}

void LP_RendererCamImpl::SetRight(float Right)
{
    m_Right = Right;
}

float LP_RendererCamImpl::Top() const
{
    return m_Top;
}

void LP_RendererCamImpl::SetTop(float top)
{
    m_Top = top;
}

float LP_RendererCamImpl::Bottom() const
{
    return m_Bottom;
}

void LP_RendererCamImpl::SetBottom(float Bottom)
{
    m_Bottom = Bottom;
}

float LP_RendererCamImpl::Near() const
{
    return m_Near;
}

void LP_RendererCamImpl::SetNear(float Near)
{
    m_Near = Near;
}

float LP_RendererCamImpl::Far() const
{
    return m_Far;
}

void LP_RendererCamImpl::SetFar(float Far)
{
    m_Far = Far;
}

QVector3D LP_RendererCamImpl::Up() const
{
    return m_Up;
}

void LP_RendererCamImpl::SetUp(const QVector3D &up)
{
    m_Up = up;
}

QVector3D LP_RendererCamImpl::Position() const
{
    return m_Position;
}

void LP_RendererCamImpl::SetPosition(const QVector3D &Position)
{
    m_Position = Position;
}

void LP_RendererCamImpl::Imitate(const LP_RendererCam o)
{
    m_bPerspective = o->IsPerspective();
    m_ResolutionX = o->ResolutionX();
    m_ResolutionY = o->ResolutionY();
    m_FOV = o->FOV();
    m_Left = o->Left();
    m_Right = o->Right();
    m_Top = o->Top();
    m_Bottom = o->Bottom();
    m_Near = o->Near();
    m_Far = o->Far();
    m_Diagonal = o->Diagonal();

    m_Target = o->Target();
    m_Position = o->Position();
    m_Up = o->Up();
    m_Pan = o->Pan();
    m_Roll = o->Roll();
}

QVector3D LP_RendererCamImpl::Target() const
{
    return m_Target;
}

void LP_RendererCamImpl::SetTarget(const QVector3D &Target)
{
    m_Target = Target;
}

void LP_RendererCamImpl::initialize()
{
    m_bPerspective = false;
    m_FOV           = 39.f;
    m_Left          = m_Bottom  = -1.f;
    m_Right         = m_Top =    1.f;
    m_Near          = 0.1f;
    m_Diagonal      = sqrtf(2.);
    m_Far           = 1000.f;

    m_Target        = QVector3D();
    m_Position      = QVector3D(0.f,0.f,10.f);
    m_Pan  = QVector3D(0.f,0.f,0.f);
    m_Roll  = QVector3D(0.f,0.f,0.f);
    m_Up      = QVector3D(0.f,1.f,0.f);
    m_ResolutionX   = m_ResolutionY = 400;
}
float LP_RendererCamImpl::Diagonal() const
{
    return m_Diagonal;
}

void LP_RendererCamImpl::setDiagonal(float Diagonal)
{
    m_Diagonal = Diagonal;
}

QVector3D LP_RendererCamImpl::Roll() const
{
    return m_Roll;
}

void LP_RendererCamImpl::SetRoll(const QVector3D &Roll)
{
    m_Roll = Roll;
}

QMatrix4x4 LP_RendererCamImpl::ProjectionMatrix() const
{
    QMatrix4x4 proj;
    proj.translate( Pan());
    if ( IsPerspective()){
        proj.perspective(FOV(), Aspect(), Near(), Far());
    }
    else{
        proj.ortho(Left(), Right(), Bottom(), Top(), Near(), Far());
    }

    return proj;
}

QMatrix4x4 LP_RendererCamImpl::ViewMatrix() const
{
    QMatrix4x4 view;
    view.translate( Roll());
    view.lookAt( Position(), Target(), Up());
    return view;
}

QMatrix4x4 LP_RendererCamImpl::ViewportMatrix() const
{
    QMatrix4x4 vp;
    vp.viewport(0, 0, ResolutionX(), ResolutionY());
    return vp;
}

void LP_RendererCamImpl::ResizeFilm(const int &w, const int &h)
{
    auto oldAspect = Aspect();

    m_ResolutionX = w;
    m_ResolutionY = h;

    if ( m_bPerspective ){
        SynchronizeProjection();
    }else{
        auto n = Aspect() / oldAspect;
        m_Left *= n;
        m_Right *= n;
    }
}

void LP_RendererCamImpl::RefreshCamera()
{
    float   hdiag(0.5*m_Diagonal);

    float   zL  = hdiag/qTan(qDegreesToRadians(.5f*m_FOV));
    float   aspect  = m_ResolutionX/double(m_ResolutionY);

    //pCam->setDiagonal( hdiag );
    m_Near = qMax(.1f, zL - 2.0f*hdiag);

    auto centre = m_Target;
    m_Position = QVector3D(centre.x(), centre.y(), centre.z() + zL );
    m_Up = QVector3D(0.f, 1.f, 0.f);
    m_Far = m_Near + 4.f*hdiag;

    m_Pan = QVector3D(0.f,0.f,0.f);
    m_Roll = QVector3D(0.f,0.f,0.f);
    m_Top = hdiag;
    m_Bottom = -hdiag;
    m_Left = -aspect*hdiag;
    m_Right = aspect*hdiag;
}

void LP_RendererCamImpl::Reset()
{
    initialize();
}

void LP_RendererCamImpl::SynchronizeProjection()
{
    if ( m_bPerspective ){
        float zL    = .5f*( m_Far + m_Near );
        float top   = zL*qTan(qDegreesToRadians(.5f*m_FOV));
        float aspect = Aspect();
        m_Top   = top;
        m_Bottom = -top;
        m_Right = aspect*top;
        m_Left  = -m_Right;
    }else{

    }
}

QVector3D LP_RendererCamImpl::Pan() const
{
    return m_Pan;
}

void LP_RendererCamImpl::SetPan(const QVector3D &Pan)
{
    m_Pan = Pan;
}

double LP_RendererCamImpl::Aspect() const
{
    return m_ResolutionX/double(m_ResolutionY);
}

int LP_RendererCamImpl::ResolutionX() const
{
    return m_ResolutionX;
}

void LP_RendererCamImpl::SetResolutionX(int ResolutionX)
{
    m_ResolutionX = ResolutionX;
}

int LP_RendererCamImpl::ResolutionY() const
{
    return m_ResolutionY;
}

void LP_RendererCamImpl::SetResolutionY(int ResolutionY)
{
    m_ResolutionY = ResolutionY;
}


