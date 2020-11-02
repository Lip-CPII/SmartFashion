#include "lp_cmd_addentity.h"


LP_Cmd_AddEntity::LP_Cmd_AddEntity(LP_Command *parent) : LP_Command(parent)
{
    setText("Add Entity");
}

void LP_Cmd_AddEntity::undo()
{
}

void LP_Cmd_AddEntity::redo()
{
}
