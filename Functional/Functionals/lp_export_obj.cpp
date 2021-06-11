#include "lp_export_obj.h"


#include "Commands/lp_commandmanager.h"
#include "Commands/lp_cmd_export_obj.h"

#include "renderer/lp_glselector.h"

#include <QFileDialog>
#include <QMap>

REGISTER_FUNCTIONAL_IMPLEMENT(LP_Export_Obj, Wavefroont (obj), menuFile/menuExport)

LP_Export_Obj::LP_Export_Obj(QObject *parent) : LP_Functional(parent)
{

}

bool LP_Export_Obj::Run()
{
    LP_OpenMesh mesh;

    static auto _isMesh = [](LP_Objectw obj){
        if ( obj.expired()){
            return LP_OpenMeshw();
        }
        return LP_OpenMeshw() = std::static_pointer_cast<LP_OpenMeshImpl>(obj.lock());
    };

    auto objs = g_GLSelector->Objects();

    for ( auto &o : objs ){
        auto c = _isMesh(o);
        if ( c.lock()){
            mesh = std::static_pointer_cast<LP_OpenMeshImpl>(o.lock());
            break;    //Since event filter has been called
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
