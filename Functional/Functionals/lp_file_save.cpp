#include "lp_file_save.h"


#include "Commands/lp_commandmanager.h"
#include "lp_document.h"

#include <QFileDialog>
#include <QMap>

REGISTER_FUNCTIONAL_IMPLEMENT(LP_File_Save, Save, menuFile)

LP_File_Save::LP_File_Save(QObject *parent) : LP_Functional(parent)
{

}


bool LP_File_Save::Run()
{
    auto pDoc = &LP_Document::gDoc;
    auto filename = pDoc->FileName();
    if ( filename.isEmpty()){
        filename = QFileDialog::getSaveFileName(0,tr("Save"), "",
                                                 tr("Project File (*.ss)"));
    }

    if ( filename.isEmpty()){
        return false;
    }

    QFile file(filename);
    if ( !file.open(QIODevice::WriteOnly)){
        return false;
    }

    QDataStream out(&file);

    auto stack = LP_CommandGroup::gCommandGroup->ActiveStack();

    bool rc = stack->Save(out);
    file.close();
    return rc;
}
