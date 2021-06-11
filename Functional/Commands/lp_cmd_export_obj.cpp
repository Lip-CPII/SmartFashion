#include "lp_cmd_export_obj.h"
#include <QDir>
#include <OpenMesh/Core/IO/MeshIO.hh>

LP_Cmd_Export_Obj::LP_Cmd_Export_Obj(LP_Command *parent) : LP_Command(parent)
{
    setText(QObject::tr("Export Wavefront OBJ"));
}

void LP_Cmd_Export_Obj::undo()
{
    if ( mStatus == 0 ){
        return;
    }
}

void LP_Cmd_Export_Obj::redo()
{
    if ( mStatus == 0 ){
        return;
    }
    MyMesh mesh_ = mEnt_Mesh->Mesh();
    OpenMesh::IO::Options opt_;
    if ( mesh_->has_vertex_texcoords2D()){
        opt_ += OpenMesh::IO::Options::VertexTexCoord;
    }
    if ( mesh_->has_vertex_normals()){
        opt_ += OpenMesh::IO::Options::VertexNormal;
    }

    bool rc = OpenMesh::IO::write_mesh(*mesh_.get(), mFile.toStdString(), opt_ );

    if ( !rc ){
        qDebug() << "Write failed";
        setObsolete(true);
        mStatus = -1;
        return;
    }

    mStatus = 0;
}

bool LP_Cmd_Export_Obj::VerifyInputs() const
{
    QFileInfo info(mFile);
    return info.dir().exists() && mEnt_Mesh;
}

bool LP_Cmd_Export_Obj::Save(QDataStream &o) const
{
    //Save all data required using
    QMap<QString, QVariant> map;
    map["mFile"] = mFile;
    map["mEntUid"] = mEnt_Mesh->Uuid();

    //===========================================================
    o << map;
    return true;
}

bool LP_Cmd_Export_Obj::Load(QDataStream &in)
{
    QString tmp;
    QMap<QString, QVariant> map;
    in >> map;

    //Parse the data
    mFile = map["mFile"].value<QString>();
    mEntUid = map["mEntUid"].value<QString>().toStdString();

    return true;
}

void LP_Cmd_Export_Obj::SetFile(const QString &file)
{
    mFile = file;
}

void LP_Cmd_Export_Obj::SetMesh(std::shared_ptr<LP_OpenMeshImpl> &&mesh)
{
    mEnt_Mesh = mesh;
}
