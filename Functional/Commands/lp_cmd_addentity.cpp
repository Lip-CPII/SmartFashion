#include "lp_cmd_addentity.h"
#include "lp_document.h"

LP_Cmd_AddEntity::LP_Cmd_AddEntity(LP_Command *parent) : LP_Command(parent)
{
    setText("Add Entity");
}

void LP_Cmd_AddEntity::undo()
{
    if ( !mObject ){
        qDebug() << "Unknow object";
        setObsolete(true);
        return;
    }
    LP_Document::gDoc.RemoveObject(mObject);
}

void LP_Cmd_AddEntity::redo()
{
    if ( !mObject ){
        mStatus = -1;
        qDebug() << "Unknow object";
        setObsolete(true);
        return;
    }
    LP_Document::gDoc.AddObject(mObject);
}

bool LP_Cmd_AddEntity::VerifyInputs() const
{
    return mObject ? true : false;
}

void LP_Cmd_AddEntity::SetEntity(LP_Object &&o)
{
    mObject = o;
}
