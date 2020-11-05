#ifndef LP_OBJECT_H
#define LP_OBJECT_H

#include "Model_global.h"
#include <memory>
#include <QSet>
#include <QUuid>
#include <QVariant>
#include <QReadWriteLock>
#include <QDebug>

class QOpenGLContext;
class QOpenGLFramebufferObject;
class QSurface;
class QOpenGLShaderProgram;
class LP_ObjectImpl;
typedef std::shared_ptr<LP_ObjectImpl> LP_Object;
typedef std::weak_ptr<LP_ObjectImpl> LP_Objectw;


#define REGISTER_OBJECT(n) \
    private: \
    inline static QByteArray mTypeName = #n; \
    QByteArray TypeName() const override {return mTypeName;}\

class LP_RendererCamImpl;
typedef std::shared_ptr<LP_RendererCamImpl> LP_RendererCam;

class MODEL_EXPORT LP_ObjectImpl : public std::enable_shared_from_this<LP_ObjectImpl>
{
public:
    virtual ~LP_ObjectImpl();


    virtual bool DrawSetup(QOpenGLContext *ctx, QSurface *surf, QVariant &option);

    virtual void Draw(QOpenGLContext *ctx,
                      QSurface *surf,
                      QOpenGLFramebufferObject *fbo,
                      const LP_RendererCam &cam,
                      QVariant &option);

    /**
     * @brief DrawSelect
     * @param ctx
     * @param surf
     * @param fbo
     * @param prog
     * @param cam
     */
    virtual void DrawSelect(
            QOpenGLContext *ctx,
            QSurface *surf,
            QOpenGLFramebufferObject *fbo,
            QOpenGLShaderProgram *prog,
            const LP_RendererCam &cam);

    virtual bool DrawCleanUp(QOpenGLContext *ctx, QSurface *surf);
    


    QUuid Uuid() const;
    void SetUuid(const QUuid &id);

    inline static LP_Object Create(LP_Object parent = nullptr){
        return std::shared_ptr<LP_ObjectImpl>(new LP_ObjectImpl(parent));
        //return std::make_shared<LP_ObjectImpl>(std::forward<LP_Object>(parent));
    }

    virtual QByteArray TypeName() const {return "Base";}


protected:
    explicit LP_ObjectImpl(LP_Objectw parent = LP_Objectw());

    LP_Objectw mParent;
    QSet<LP_Objectw> mChildren;
    QReadWriteLock mLock;
private:
    QUuid mUid;


    friend class LP_Document;
};

inline bool operator ==(const LP_Objectw &a, const LP_Objectw &b){
    return a.lock()->Uuid() == b.lock()->Uuid();
}

inline uint qHash(const LP_Objectw &c){
        return (qHash(c.lock()->Uuid()));
    }

inline QDebug operator<<(QDebug debug, const LP_Objectw& o){
        QDebugStateSaver saver(debug);
        auto _o = o.lock();
        if ( _o ){
            debug.nospace() << _o->TypeName() << "-(" << _o->Uuid() << ')';
        }else{
            debug.quote() << "Null LP_Object";
        }

        return debug;
    }
Q_DECLARE_METATYPE(LP_Object)

#endif // LP_OBJECT_H
