#include "lp_document.h"
#include "lp_object.h"
#include <QDebug>

LP_Document LP_Document::gDoc;

LP_Document::LP_Document(QObject *parent) : QObject(parent)
{
    static auto id = qRegisterMetaType<LP_Objectw>("LP_Objectw");
//    qDebug() << id;
}

LP_Document::~LP_Document()
{
    mObjects.clear();
}

void LP_Document::AddObject(LP_Objectw &&ent, LP_Objectw &&parent)
{
    QWriteLocker locker(&mLock);
    if ( auto p = parent.lock()){
        p->mChildren.insert(ent);
    }else{
        mObjects.insert(ent);
    }
    emit updateTreeView();
    emit requestGLSetup(ent);
}

void LP_Document::RemoveObject(LP_Objectw &&ent, LP_Objectw &&parent)
{
    QWriteLocker locker(&mLock);
    if ( auto p = parent.lock()){
        p->mChildren.remove(ent);
    }else{
        if (!mObjects.remove(ent)){
            mHiddens.remove(ent);
        }
    }

    emit updateTreeView();
    emit requestGLCleanup(ent);
}

void LP_Document::HideObject(const QUuid &id)
{
    QWriteLocker locker(&mLock);
    auto o = FindObject(id, &mObjects);
    if ( o.lock()){
        mObjects.remove(o);
        mHiddens.insert(o);
    }
}

void LP_Document::ShowObject(const QUuid &id)
{
    QWriteLocker locker(&mLock);
    auto o = FindObject(id, &mHiddens);
    if ( o.lock()){
        mHiddens.remove(o);
        mObjects.insert(o);
    }
}

QStandardItemModel *LP_Document::ToQStandardModel() const
{
    mModel->clear();

    QStringList headers = {"Items", "Hide","ID"};
    mModel->setHorizontalHeaderLabels(headers);

    std::function<QStandardItem*(LP_Object&&)> parse;


    parse = [&](LP_Object &&o){
        auto cur = new QStandardItem(o->TypeName().data());

        cur->setToolTip(o->Uuid().toString());
        cur->setData(QVariant::fromValue(o), Qt::UserRole + 1);
        cur->setData(o->Uuid(), Qt::UserRole + 2);

        for ( auto c : o->mChildren ){
            auto cp = parse(c.lock());
            cur->appendRow(cp);
        }
        return cur;
    };

    QReadLocker locker(&mLock);

    for ( auto o : mObjects){
        mModel->appendRow( {parse(o.lock()),
                           new QStandardItem(QChar(0x25C9)),
                           new QStandardItem(o.lock()->Uuid().toString())});
    }
    for ( auto o : mHiddens){
        mModel->appendRow( {parse(o.lock()),
                           new QStandardItem(QChar(0x25CE)),
                           new QStandardItem(o.lock()->Uuid().toString())});
    }
    return mModel.get();
}

const QSet<LP_Objectw>& LP_Document::Objects() const
{
    return mObjects;
}

const QSet<LP_Objectw> &LP_Document::Hiddens() const
{
    return mHiddens;
}

void LP_Document::ResetDocument()
{
    for ( auto o : mObjects ){
        o.reset();
    }
    mObjects.clear();
}

QReadWriteLock &LP_Document::Lock() const
{
    return mLock;
}

QString LP_Document::FileName() const
{
    return mFileName;
}

void LP_Document::SetFileName(const QString &fileName)
{
    mFileName = fileName;
}
