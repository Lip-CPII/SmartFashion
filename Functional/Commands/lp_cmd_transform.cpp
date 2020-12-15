#include "lp_cmd_transform.h"
#include "lp_geometry.h"

#include "lp_document.h"
#include "renderer/lp_glrenderer.h"

LP_Cmd_Transform::LP_Cmd_Transform(LP_Command *parent) : LP_Command(parent)
{
    setText(QObject::tr("Transform Geometry"));
}

void LP_Cmd_Transform::undo()
{
    auto ow = LP_Document::gDoc.FindObject(mGeoUid);
    auto o = ow.lock();
    if ( !o ){
        qDebug() << "Unknow object id: " << mGeoUid;
        setObsolete(true);
    }
    auto geo = std::static_pointer_cast<LP_GeometryImpl>(o);
    if ( !geo ){
        qDebug() << "Object is not geometry";
        setObsolete(true);
    }

    geo->SetModelTrans(mOrgTrans);
    LP_GLRenderer::UpdateAll();
}

void LP_Cmd_Transform::redo()
{
    auto ow = LP_Document::gDoc.FindObject(mGeoUid);
    auto o = ow.lock();
    if ( !o ){
        qDebug() << "Unknow object id: " << mGeoUid;
        setObsolete(true);
    }
    auto geo = std::static_pointer_cast<LP_GeometryImpl>(o);
    if ( !geo ){
        qDebug() << "Object is not geometry";
        setObsolete(true);
    }

    mOrgTrans = geo->ModelTrans();
    geo->SetModelTrans(mTrans);
    LP_GLRenderer::UpdateAll();
}

bool LP_Cmd_Transform::VerifyInputs() const
{
    bool rc(false);
    auto inv = mTrans.inverted(&rc);
    Q_UNUSED(inv)
    if ( !rc ){
        qDebug() << "Invalid transformation!";
        return false;
    }

    auto ow = LP_Document::gDoc.FindObject(mGeoUid);
    auto o = ow.lock();
    if ( !o ){
        qDebug() << "Unknow object id: " << mGeoUid;
        return false;
    }
    auto geo = std::static_pointer_cast<LP_GeometryImpl>(o);
    if ( !geo ){
        qDebug() << "Object is not geometry";
        return false;
    }

    return true;
}

bool LP_Cmd_Transform::Save(QDataStream &o) const
{
    //Save all data required using
    QMap<QString, QVariant> map;
    map["mTrans"] = mTrans;
    map["mGeoUid"] = mGeoUid;

    //===========================================================
    o << map;
    return true;
}

bool LP_Cmd_Transform::Load(QDataStream &in)
{
    QString tmp;
    QMap<QString, QVariant> map;
    in >> map;

    //Parse the data
    mTrans = map["mTrans"].value<QMatrix4x4>();
    mGeoUid = map["mGeoUid"].value<QString>();

    return true;
}

void LP_Cmd_Transform::SetGeometry(QString uuid)
{
    mGeoUid = uuid;
}

void LP_Cmd_Transform::SetTrans(const QMatrix4x4 &trans)
{
    mTrans = trans;
}


