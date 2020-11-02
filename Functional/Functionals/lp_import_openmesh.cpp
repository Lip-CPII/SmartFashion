#include "lp_import_openmesh.h"

#include "Commands/lp_commandmanager.h"
#include "Commands/lp_cmd_import_opmesh.h"

#include <QFileDialog>
#include <QMap>

REGISTER_FUNCTIONAL_IMPLEMENT(LP_Import_OpenMesh, Open Mesh, menuFile/menuImport)

LP_Import_OpenMesh::LP_Import_OpenMesh(QObject *parent) : LP_Functional(parent)
{

}

QWidget *LP_Import_OpenMesh::DockUi()
{
    return nullptr;
}

bool LP_Import_OpenMesh::Run()
{
    auto file = QFileDialog::getOpenFileName(0,tr("Import OpenMesh"), "",
                                             tr("Mesh (*.obj *.stl)"));

    if ( file.isEmpty()){
        return false;
    }

    auto cmd = new LP_Cmd_Import_OpMesh;
    cmd->SetFile(file);
    if ( !cmd->VerifyInputs()){
        delete cmd;
        return false;
    }
    LP_CommandGroup::gCommandGroup->ActiveStack()->Push(cmd);   //Execute the command

    return true;
}

