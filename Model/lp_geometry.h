#ifndef LP_GEOMETRY_H
#define LP_GEOMETRY_H


#include "lp_object.h"
#include <QVector3D>

class LP_GeometryImpl;
typedef std::shared_ptr<LP_GeometryImpl> LP_Geometry;
typedef std::weak_ptr<LP_GeometryImpl> LP_Geometryw;

class MODEL_EXPORT LP_GeometryImpl : public LP_ObjectImpl
{
    REGISTER_OBJECT(Geometry)
public:
    inline static LP_Geometry Create(LP_Object parent = nullptr) {
        return LP_Geometry(new LP_GeometryImpl(parent));
    }

    virtual void BoundingBox(QVector3D &bbmin, QVector3D &bbmax);
    virtual void SetBoundingBox(const QVector3D &min, const QVector3D &max);

    virtual ~LP_GeometryImpl();

    void _Dump(QDebug &debug) override {
        LP_ObjectImpl::_Dump(debug);
        debug.nospace() << "Geometry : Bounding Box (" << mBBmin << ", " << mBBmax << ")"
                        << "\n";
    }
protected:
    explicit LP_GeometryImpl(LP_Object parent = nullptr);


    bool mGLInitialized;
    QVector3D mBBmin;
    QVector3D mBBmax;
};

#endif // LP_GEOMETRY_H
