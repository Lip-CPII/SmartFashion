#include "lp_export_obj.h"


#include "Commands/lp_commandmanager.h"
#include "Commands/lp_cmd_export_obj.h"
#include "lp_pointcloud.h"

#include "renderer/lp_glselector.h"

#include <QFileDialog>
#include <QRandomGenerator>
#include <QMap>

REGISTER_FUNCTIONAL_IMPLEMENT(LP_Export_Obj, Wavefroont (obj), menuFile/menuExport)

LP_Export_Obj::LP_Export_Obj(QObject *parent) : LP_Functional(parent)
{

}

bool LP_Export_Obj::Run()
{
    LP_OpenMesh mesh;

    auto objs = g_GLSelector->Objects();

    for ( auto &o : objs ){
        if ( o.expired()){
            continue;
        }
        if ( LP_OpenMeshImpl::mTypeName == o.lock()->TypeName()){
            mesh = std::static_pointer_cast<LP_OpenMeshImpl>(o.lock());
            break;    //Since event filter has been called
        } else if ( LP_PointCloudImpl::mTypeName == o.lock()->TypeName()){

            MyMesh m = std::make_shared<OpMesh>();
            auto pc = std::static_pointer_cast<LP_PointCloudImpl>(o.lock());
            auto &pts = pc->Points();
            auto &norms = pc->Normals();
            auto nv = pts.size();
            for ( size_t i=0; i<nv; ++i){
                auto &pt = pts[i],
                     &nl = norms[i];
                OpMesh::Point p;
                OpMesh::Normal n;
                p[0] = pt.x();
                p[1] = pt.y();
                p[2] = pt.z();
                n[0] = nl.x();
                n[1] = nl.y();
                n[2] = nl.z();
                auto vh = m->add_vertex(p);
                m->set_normal(vh, n);
            }
            mesh = LP_OpenMeshImpl::Create(nullptr);
            mesh->SetMesh(m);
            break;
        }
    }

    if ( !mesh ){
        qInfo() << "No object selected";
        return false;
    }


    auto file = QFileDialog::getSaveFileName(0,tr("Export OBJ"), "",
                                             tr("Wavefront (*.obj)"));

    if ( file.isEmpty()){
        return false;
    }

    auto cmd = new LP_Cmd_Export_Obj;
    cmd->SetFile(file);
    cmd->SetMesh(std::move(mesh));
    if ( !cmd->VerifyInputs()){
        delete cmd;
        return false;
    }
    LP_CommandGroup::gCommandGroup->ActiveStack()->Push(cmd);   //Execute the command

    return true;
}

QWidget *LP_Export_Obj::DockUi()
{
    return nullptr;
}
