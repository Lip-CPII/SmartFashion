#include "lp_import_pointcloud.h"

#include "Commands/lp_commandmanager.h"
#include "Commands/lp_cmd_import_pointcloud.h"

#include <QFileDialog>
#include <QMap>

REGISTER_FUNCTIONAL_IMPLEMENT(LP_Import_PointCloud, Point Cloud, menuFile/menuImport)

LP_Import_PointCloud::LP_Import_PointCloud(QObject *parent) : LP_Functional(parent)
{

}

QWidget *LP_Import_PointCloud::DockUi()
{
    return nullptr;
}

bool LP_Import_PointCloud::Run()
{
    auto file = QFileDialog::getOpenFileName(0,tr("Import Point Cloud"), "",
                                             tr("PointCloud(*.obj)"));

    if ( file.isEmpty()){
        return false;
    }

    auto cmd = new LP_Cmd_Import_PointCloud;
    cmd->SetFile(file);
    if ( !cmd->VerifyInputs()){
        delete cmd;
        return false;
    }
    LP_CommandGroup::gCommandGroup->ActiveStack()->Push(cmd);   //Execute the command

    return true;
}

