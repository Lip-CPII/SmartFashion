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
    void HideObject(const QUuid &id);
    void ShowObject(const QUuid &id);

    inline LP_Objectw FindObject(const QUuid &id, const QSet<LP_Objectw> *list = nullptr){
        static LP_Object proxy = LP_ObjectImpl::Create();
        proxy->mUid = id;
        if ( !list ){
            list = &mObjects;
        }
        auto obj = list->find(proxy);
        if ( obj == list->cend()){
            return LP_Objectw();
        }
        return *obj;
    }

    QStandardItemModel *ToQStandardModel() const;
    const QSet<LP_Objectw> &Objects() const;
    const QSet<LP_Objectw> &Hiddens() const;

    void ResetDocument();
    QReadWriteLock& Lock() const;

    static LP_Document gDoc;    //TODO multi doc

    QString FileName() const;
    void SetFileName(const QString &fileName);

protected:
    QSet<LP_Objectw> mObjects;
    QSet<LP_Objectw> mHiddens;
signals:
    void updateTreeView();
    void requestGLSetup(LP_Objectw);
    void requestGLCleanup(LP_Objectw);

private:
    mutable QReadWriteLock mLock;
    QString mFileName;
    std::unique_ptr<QStandardItemModel> mModel = std::make_unique<QStandardItemModel>();
};

#endif // LP_DOCUMENT_H
