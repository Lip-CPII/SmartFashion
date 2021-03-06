#ifndef LP_POINTCLOUD_H
#define LP_POINTCLOUD_H

#include "lp_geometry.h"

#include <QOpenGLBuffer>
#include <QOpenGLTexture>


class QOpenGLShaderProgram;

class LP_PointCloudImpl;
typedef std::shared_ptr<LP_PointCloudImpl> LP_PointCloud;
typedef std::weak_ptr<LP_PointCloudImpl> LP_PointCloudw;

class MODEL_EXPORT LP_PointCloudImpl : public LP_GeometryImpl
{
    REGISTER_OBJECT(PointCloud)
public:
    inline static LP_PointCloud Create(LP_Object parent = nullptr) {
        return LP_PointCloud(new LP_PointCloudImpl(parent));
    }
    virtual ~LP_PointCloudImpl();

    void SetPoints(std::vector<QVector3D> &&pc);
    const std::vector<QVector3D> & Points() const;

    void SetNormals(std::vector<QVector3D> &&pc);
    const std::vector<QVector3D> &Normals() const;

    void SetColors(std::vector<QVector3D> &&pc);
    const std::vector<QVector3D> &Colors() const;

    void _Dump(QDebug &debug) override;

protected:
    explicit LP_PointCloudImpl(LP_Object parent = nullptr);

private:
    QMap<QString, QOpenGLShaderProgram*> mPrograms;
    QOpenGLBuffer *mVBO;
    QOpenGLBuffer *mIndices;

    std::vector<QVector3D> mPoints;
    std::vector<QVector3D> mNormals;
    std::vector<QVector3D> mColors;

    // LP_ObjectImpl interface
public:
    bool DrawSetup(QOpenGLContext *ctx, QSurface *surf, QVariant &option) override;
    void Draw(QOpenGLContext *ctx, QSurface *surf, QOpenGLFramebufferObject *fbo, const LP_RendererCam &cam, QVariant &option)  override;
    bool DrawCleanUp(QOpenGLContext *ctx, QSurface *surf) override;
    void DrawSelect(QOpenGLContext *ctx, QSurface *surf, QOpenGLFramebufferObject *fbo, QOpenGLShaderProgram *prog, const LP_RendererCam &cam) override;
};

#endif // LP_POINTCLOUD_H
