#include "lp_cmd_import_pointcloud.h"
#include "lp_document.h"
#include "lp_openmesh.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <OpenMesh/Core/IO/MeshIO.hh>


LP_Cmd_Import_PointCloud::LP_Cmd_Import_PointCloud(LP_Command *parent) : LP_Command(parent)
{
    setText(QObject::tr("Import OpenMesh"));
}

void LP_Cmd_Import_PointCloud::undo()
{
    if ( mStatus == 0 ){
        LP_Document::gDoc.RemoveObject(mEnt_PC);
        return;
    }
}

void LP_Cmd_Import_PointCloud::redo()
{
    if ( mStatus == 0 ){
        LP_Document::gDoc.AddObject(mEnt_PC);
        return;
    }
//    gLogger->Log(QObject::tr("%1 - Loading file : %2").arg(text()).arg(mFile));

#ifdef TIMER_LOG
    QElapsedTimer timer;
    timer.start();
#endif

    OpenMesh::IO::Options opt_;
    //opt_ += OpenMesh::IO::Options::VertexColor;
    opt_ += OpenMesh::IO::Options::VertexNormal;

    MyMesh mesh_ = std::make_shared<OpMesh>();
//    mesh_->request_face_normals();
//    mesh_->request_face_colors();
    mesh_->request_vertex_normals();
    mesh_->request_vertex_colors();
    mesh_->request_vertex_texcoords2D();
    bool rc = OpenMesh::IO::read_mesh(*mesh_.get(), mFile.toStdString(), opt_ );

#ifdef TIMER_LOG
    qDebug() << QString("Load mesh : %1 ms").arg(timer.nsecsElapsed() * 1e-6);
    timer.restart();
#endif

    auto nv{mesh_->n_vertices()};
    if ( !rc || nv < 1 ){
        mStatus = -1;
        setObsolete(true);
        //gLogger->Log(QObject::tr("%1 - Corrupted or invalid file").arg(text()),10);
        qDebug() << QString("Import failed #v: %1").arg(nv);
        return;
    }

    auto tooltip = QObject::tr("%1 vertices\n").arg(nv);

    if ( ! opt_.check( OpenMesh::IO::Options::VertexNormal ) )
      mesh_->update_vertex_normals();
    else
      std::cout << "File provides vertex normals\n";


    // bounding box
    typename OpMesh::ConstVertexIter vIt(mesh_->vertices_begin());
    typename OpMesh::ConstVertexIter vEnd(mesh_->vertices_end());

    using OpenMesh::Vec3f;

    Vec3f bbMin, bbMax;

    bbMin = bbMax = OpenMesh::vector_cast<Vec3f>(mesh_->point(*vIt));
    std::vector<QVector3D> pts(nv), norms(nv);
    for (size_t count=0; vIt!=vEnd; ++vIt, ++count)
    {
        auto &&v = OpenMesh::vector_cast<Vec3f>(mesh_->point(*vIt));
        auto &&n = OpenMesh::vector_cast<Vec3f>(mesh_->normal(*vIt));
        bbMin.minimize( v);
        bbMax.maximize( v);
        pts[count].setX(v[0]);pts[count].setY(v[1]);pts[count].setZ(v[2]);
        norms[count].setX(n[0]);norms[count].setY(n[1]);norms[count].setZ(n[2]);
    }

    tooltip += QString("BBox max(%1, %2, %3), ").arg(bbMax[0]).arg(bbMax[1]).arg(bbMax[2]);
    tooltip += QString("min(%1, %2, %3)\n").arg(bbMin[0]).arg(bbMin[1]).arg(bbMin[2]);

    mStatus = 0;
    //gLogger->Log(QObject::tr("%1 - Successful").arg(text()));

    auto objPtr = LP_PointCloudImpl::Create();

    objPtr->SetPoints(std::move(pts));
    objPtr->SetNormals(std::move(norms));
    objPtr->SetBoundingBox(  QVector3D(bbMin[0], bbMin[1], bbMin[2]),
                             QVector3D(bbMax[0], bbMax[1], bbMax[2]));

//    mEnt_Mesh->setToolTip(mEnt_Mesh->toolTip()+"\n"+tooltip);

    //A must do step for reconstructing the history
    if ( !mEntUid.empty()){
        auto uid = QUuid(mEntUid.c_str());
        if ( !uid.isNull()){
            objPtr->SetUuid(uid);
        }
    }

    mEnt_PC = objPtr;
    LP_Document::gDoc.AddObject(mEnt_PC);
}

bool LP_Cmd_Import_PointCloud::VerifyInputs() const
{
    return QFile::exists(mFile);
}

bool LP_Cmd_Import_PointCloud::Save(QDataStream &o) const
{
    //Save all data required using
    QMap<QString, QVariant> map;
    map["mFile"] = mFile;
    map["mEntUid"] = mEnt_PC->Uuid();

    //===========================================================
    o << map;
    return true;
}

bool LP_Cmd_Import_PointCloud::Load(QDataStream &in)
{
    QMap<QString, QVariant> map;
    in >> map;

    //Parse the data
    mFile = map["mFile"].value<QString>();
    mEntUid = map["mEntUid"].value<QString>().toStdString();

    return true;
}

void LP_Cmd_Import_PointCloud::SetFile(const QString &file)
{
    mFile = file;
}
