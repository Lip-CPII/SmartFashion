#include "lp_file_open.h"

#include "Commands/lp_commandmanager.h"
#include "lp_document.h"
#include "lp_new.h"

#include <QFileDialog>
#include <QMap>

REGISTER_FUNCTIONAL_IMPLEMENT(LP_File_Open, Open, menuFile)

LP_File_Open::LP_File_Open(QObject *parent) : LP_Functional(parent)
{

}


bool LP_File_Open::Run()
{
    auto filename = QFileDialog::getOpenFileName(0,tr("Open"), "",
                                             tr("Project File (*.ss)"));

    if ( filename.isEmpty()){
        return false;
    }

    QFile file(filename);
    if ( !file.open(QIODevice::ReadOnly)){
        return false;
    }

    LP_New tmp;
    tmp.Run();  //Using the function internally

    QDataStream in(&file);
    int qver=-1;
    in >> qver;
    in.setVersion(qver);

    auto stack = LP_CommandGroup::gCommandGroup->ActiveStack();

    stack->Load(in);
    file.close();

    return true;
}
