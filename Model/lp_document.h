#ifndef LP_DOCUMENT_H
#define LP_DOCUMENT_H

#include "Model_global.h"
#include "lp_object.h"
#include <QStandardItemModel>
#include <QReadWriteLock>

class MODEL_EXPORT LP_Document : public QObject
{
    Q_OBJECT
public:
    explicit LP_Document(QObject *parent = nullptr);
    virtual ~LP_Document();

    void AddObject(LP_Objectw &&ent, LP_Objectw &&parent = LP_Objectw());
    void RemoveObject(LP_Objectw &&ent, LP_Objectw &&parent = LP_Objectw());

    inline LP_Objectw FindObject(const QUuid &id){
        static LP_Object proxy = LP_ObjectImpl::Create();
        proxy->mUid = id;
        auto obj = mObjects.find(proxy);
        if ( obj == mObjects.cend()){
            return LP_Objectw();
        }
        return *obj;
    }

    QStandardItemModel *ToQStandardModel() const;
    const QSet<LP_Objectw> &Objects() const;
    void ResetDocument();
    QReadWriteLock& Lock() const;

    static LP_Document gDoc;    //TODO multi doc

    QString FileName() const;
    void SetFileName(const QString &fileName);

protected:
    QSet<LP_Objectw> mObjects;
signals:
    void updateTreeView();
    void requestGLSetup(LP_Objectw);
    void requestGLCleanup(LP_Objectw);

private:
    mutable QReadWriteLock mLock;
    QString mFileName;
};

#endif // LP_DOCUMENT_H
