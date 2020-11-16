#include "lp_cmd_import_opmesh.h"
#include "lp_document.h"

#include <QFile>
#include <OpenMesh/Core/IO/MeshIO.hh>


LP_Cmd_Import_OpMesh::LP_Cmd_Import_OpMesh(LP_Command *parent) : LP_Command(parent)
{
    setText(QObject::tr("Import OpenMesh"));
}

void LP_Cmd_Import_OpMesh::undo()
{
    if ( mStatus == 0 ){
        LP_Document::gDoc.RemoveObject(mEnt_Mesh);
        return;
    }
}

void LP_Cmd_Import_OpMesh::redo()
{
    if ( mStatus == 0 ){
        LP_Document::gDoc.AddObject(mEnt_Mesh);
        return;
    }
//    gLogger->Log(QObject::tr("%1 - Loading file : %2").arg(text()).arg(mFile));

#ifdef TIMER_LOG
    QElapsedTimer timer;
    timer.start();
#endif

    OpenMesh::IO::Options opt_;
    opt_ += OpenMesh::IO::Options::VertexColor;

    MyMesh mesh_ = std::make_shared<OpMesh>();
//    mesh_->request_face_normals();
//    mesh_->request_face_colors();
    mesh_->request_vertex_normals();
    mesh_->request_vertex_colors();
//    mesh_->request_vertex_texcoords2D();
    bool rc = OpenMesh::IO::read_mesh(*mesh_.get(), mFile.toStdString(), opt_ );

#ifdef TIMER_LOG
    qDebug() << QString("Load mesh : %1 ms").arg(timer.nsecsElapsed() * 1e-6);
    timer.restart();
#endif

    auto nv{mesh_->n_vertices()},
         ne{mesh_->n_edges()},
         nf{mesh_->n_faces()};
    if ( !rc || nv < 3 || ne < 3 || nf < 1 ){
        mStatus = -1;
        setObsolete(true);
        //gLogger->Log(QObject::tr("%1 - Corrupted or invalid file").arg(text()),10);
        return;
    }

    auto tooltip = QObject::tr("%1 vertices, %2 edges, %3 faces\n").arg(nv).arg(ne).arg(nf);

    // update face and vertex normals
    if ( ! opt_.check( OpenMesh::IO::Options::FaceNormal ) )
      mesh_->update_face_normals();
    else
      std::cout << "File provides face normals\n";

    if ( ! opt_.check( OpenMesh::IO::Options::VertexNormal ) )
      mesh_->update_vertex_normals();
    else
      std::cout << "File provides vertex normals\n";

#ifdef TIMER_LOG
    qDebug() << QString("Rebuild mesh info : %1 ms").arg(timer.nsecsElapsed() * 1e-6);
#endif

    // bounding box
    typename OpMesh::ConstVertexIter vIt(mesh_->vertices_begin());
    typename OpMesh::ConstVertexIter vEnd(mesh_->vertices_end());

    using OpenMesh::Vec3f;

    Vec3f bbMin, bbMax;

    bbMin = bbMax = OpenMesh::vector_cast<Vec3f>(mesh_->point(*vIt));

    for (size_t count=0; vIt!=vEnd; ++vIt, ++count)
    {
      bbMin.minimize( OpenMesh::vector_cast<Vec3f>(mesh_->point(*vIt)));
      bbMax.maximize( OpenMesh::vector_cast<Vec3f>(mesh_->point(*vIt)));
    }
    if ( ! opt_.check( OpenMesh::IO::Options::VertexColor ) ){
        for (vIt = mesh_->vertices_begin(); vIt!=vEnd; ++vIt){
            mesh_->set_color(*vIt, OpMesh::Color(52, 52, 52));
        }
    }
    else
      std::cout << "File provides vertex color\n";

    tooltip += QString("BBox max(%1, %2, %3), ").arg(bbMax[0]).arg(bbMax[1]).arg(bbMax[2]);
    tooltip += QString("min(%1, %2, %3)\n").arg(bbMin[0]).arg(bbMin[1]).arg(bbMin[2]);

    mStatus = 0;
    //gLogger->Log(QObject::tr("%1 - Successful").arg(text()));

    auto objPtr = LP_OpenMeshImpl::Create();

    objPtr->SetMesh(mesh_);
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

    mEnt_Mesh = objPtr;
    LP_Document::gDoc.AddObject(mEnt_Mesh);
}

bool LP_Cmd_Import_OpMesh::VerifyInputs() const
{
    return QFile::exists(mFile);
}

void LP_Cmd_Import_OpMesh::SetFile(const QString &file)
{
    mFile = file;
}

bool LP_Cmd_Import_OpMesh::Save(QDataStream &o) const
{
    //Save all data required using
    QMap<QString, QVariant> map;
    map["mFile"] = mFile;
    map["mEntUid"] = mEnt_Mesh->Uuid();

    //===========================================================
    o << map;
    return true;
}

bool LP_Cmd_Import_OpMesh::Load(QDataStream &in)
{
    QString tmp;
    QMap<QString, QVariant> map;
    in >> map;

    //Parse the data
    mFile = map["mFile"].value<QString>();
    mEntUid = map["mEntUid"].value<QString>().toStdString();

    return true;
}
