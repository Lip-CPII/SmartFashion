#ifndef LP_OPENMESH_H
#define LP_OPENMESH_H

#include "lp_geometry.h"


#include <OpenMesh/Core/Mesh/TriMesh_ArrayKernelT.hh>
#include <OpenMesh/Core/Mesh/Attributes.hh>
#include <QOpenGLBuffer>
#include <QDebug>

struct MeshTraits : public OpenMesh::DefaultTraits
{
  VertexAttributes  ( OpenMesh::Attributes::Normal       |
                      OpenMesh::Attributes::Status       );
  EdgeAttributes    ( OpenMesh::Attributes::Status       );
  HalfedgeAttributes( OpenMesh::Attributes::PrevHalfedge );
  FaceAttributes    ( OpenMesh::Attributes::Normal       |
              OpenMesh::Attributes::Status       );
};

typedef OpenMesh::TriMesh_ArrayKernelT<MeshTraits>  OpMesh;
typedef std::shared_ptr<OpMesh> MyMesh;

class QOpenGLShaderProgram;

class LP_OpenMeshImpl;
typedef std::shared_ptr<LP_OpenMeshImpl> LP_OpenMesh;
typedef std::weak_ptr<LP_OpenMeshImpl> LP_OpenMeshw;

class MODEL_EXPORT LP_OpenMeshImpl : public LP_GeometryImpl
{
    REGISTER_OBJECT(Mesh)
public:
    inline static LP_OpenMesh Create(LP_Object parent = nullptr) {
        return LP_OpenMesh(new LP_OpenMeshImpl(parent));
    }
    virtual ~LP_OpenMeshImpl();

    // LP_ObjectImpl interface
    void Draw(QOpenGLContext *ctx, QSurface *surf, QOpenGLFramebufferObject *fbo, const LP_RendererCam &cam, QVariant &option) override;
    void DrawSelect(QOpenGLContext *ctx, QSurface *surf, QOpenGLFramebufferObject *fbo, QOpenGLShaderProgram *prog, const LP_RendererCam &cam) override;
    bool DrawSetup(QOpenGLContext *ctx, QSurface *surf, QVariant &option) override;
    bool DrawCleanUp(QOpenGLContext *ctx, QSurface *surf) override;

    MyMesh Mesh() const;
    void SetMesh(const MyMesh &mesh);

    void _Dump(QDebug &debug) override;

protected:
    explicit LP_OpenMeshImpl(LP_Object parent = nullptr);


private:
    MyMesh mMesh;

    QMap<QString, QOpenGLShaderProgram*> mPrograms;
    std::shared_ptr<QOpenGLShaderProgram> mProgramBoundary;
    QOpenGLBuffer *mVBO;
    QOpenGLBuffer *mIndices;

    size_t mStrides[3];
};


#endif // LP_OPENMESH_H
