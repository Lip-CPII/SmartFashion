#ifndef LP_CMD_ADDENTITY_H
#define LP_CMD_ADDENTITY_H

#include "Functional_global.h"
#include "Commands/lp_command.h"
#include "lp_object.h"

class FUNCTIONAL_EXPORT LP_Cmd_AddEntity : public LP_Command
{
    REGISTER(LP_Command, LP_Cmd_AddEntity)
public:
    explicit LP_Cmd_AddEntity(LP_Command *parent = nullptr);

    // QUndoCommand interface
    void undo() override;
    void redo() override;

    // LP_Command interface
    bool VerifyInputs() const override;

    void SetEntity(LP_Object &&o);

private:
    LP_Object mObject;
};

#endif // LP_CMD_ADDENTITY_H
