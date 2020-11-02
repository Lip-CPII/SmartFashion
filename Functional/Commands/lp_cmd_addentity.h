#ifndef LP_CMD_ADDENTITY_H
#define LP_CMD_ADDENTITY_H

#include "Functional_global.h"
#include "Commands/lp_command.h"

class FUNCTIONAL_EXPORT LP_Cmd_AddEntity : public LP_Command
{
    REGISTER(LP_Command, LP_Cmd_AddEntity)
public:
    explicit LP_Cmd_AddEntity(LP_Command *parent = nullptr);

    // QUndoCommand interface
    void undo() override;
    void redo() override;
};

#endif // LP_CMD_ADDENTITY_H
