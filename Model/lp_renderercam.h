#ifndef LP_RENDERERCAM_H
#define LP_RENDERERCAM_H

#include "lp_object.h"
#include <QMatrix4x4>

class LP_RendererCamImpl;
typedef std::shared_ptr<LP_RendererCamImpl> LP_RendererCam;

class MODEL_EXPORT LP_RendererCamImpl : public LP_ObjectImpl
{
public:
    inline static LP_RendererCam Create(LP_Object parent) {
        return LP_RendererCam(new LP_RendererCamImpl(parent));
    }

    void Imitate(const LP_RendererCam o);

    QVector3D Target() const;
    void SetTarget(const QVector3D &Target);

    QVector3D Position() const;
    void SetPosition(const QVector3D &Position);

    QVector3D Up() const;
    void SetUp(const QVector3D &CamUp);

    float Far() const;
    void SetFar(float Far);

    float Near() const;
    void SetNear(float Near);

    float Diagonal() const;
    void setDiagonal(float Diagonal);

    float Bottom() const;
    void SetBottom(float Bottom);

    float Top() const;
    void SetTop(float top);

    float Right() const;
    void SetRight(float Right);

    float Left() const;
    void SetLeft(float Left);

    float FOV() const;
    void SetFOV(float FOV);

    bool IsPerspective() const;
    void SetPerspective(bool bPerspective);

    int ResolutionY() const;
    void SetResolutionY(int ResolutionY);

    int ResolutionX() const;
    void SetResolutionX(int ResolutionX);

    QVector3D Pan() const;
    void SetPan(const QVector3D &Pan);

    double Aspect() const;

    QVector3D Roll() const;
    void SetRoll(const QVector3D &Roll);

    QMatrix4x4 ProjectionMatrix() const;
    QMatrix4x4 ViewMatrix() const;
    QMatrix4x4 ViewportMatrix() const;
    void ResizeFilm(const int &w, const int &h);
    void RefreshCamera();

    void    Reset();
    void    SynchronizeProjection();
signals:

public slots:

protected:
    void    initialize();

protected:
    bool    m_bPerspective;     //False
    int     m_ResolutionX;      //100,100
    int     m_ResolutionY;      //100,100
    float   m_FOV;              //59.f
    float   m_Left;             //-1.f;
    float   m_Right;            //1.f
    float   m_Top;               //1.f
    float   m_Bottom;           //-1.ff
    float   m_Near;             //0.01f
    float   m_Far;              //1000.f
    float   m_Diagonal;         //sqrt(2.f)

    QVector3D   m_Target;       //(0,0,0)
    QVector3D   m_Pan;          //(0,0,0)
    QVector3D   m_Roll;         //(0,0,0) The vector in rolling the camera forward or backward
    QVector3D   m_Position;     //(0,0,10);
    QVector3D   m_Up;        //(0,1,0)

    explicit LP_RendererCamImpl(LP_Object parent);
};


#endif // LP_RENDERERCAM_H
