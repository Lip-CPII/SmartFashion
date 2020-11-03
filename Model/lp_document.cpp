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
        mObjects.remove(ent);
    }
    emit updateTreeView();
    emit requestGLCleanup(ent);
}

QStandardItemModel *LP_Document::ToQStandardModel() const
{
    auto model = new QStandardItemModel();
    QStringList headers = {"Items", "ID"};
    model->setHorizontalHeaderLabels(headers);

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
        model->appendRow( parse(o.lock()));
    }

    return model;
}

const QSet<LP_Objectw>& LP_Document::Objects() const
{
    return mObjects;
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
