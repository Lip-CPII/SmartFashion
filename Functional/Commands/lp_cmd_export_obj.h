#ifndef LP_CMD_EXPORT_OBJ_H
#define LP_CMD_EXPORT_OBJ_H

#include "lp_command.h"
#include "lp_openmesh.h"


class FUNCTIONAL_EXPORT LP_Cmd_Export_Obj : public LP_Command
{
    REGISTER(LP_Command, LP_Cmd_Export_Obj)
public:
    explicit LP_Cmd_Export_Obj(LP_Command *parent = nullptr);


    // QUndoCommand interface
    void undo() override;
    void redo() override;

    // LP_Command interface
    bool VerifyInputs() const override;
    bool Save(QDataStream &o) const override;
    bool Load(QDataStream &in) override;

    void SetFile(const QString &file);
    void SetMesh(std::shared_ptr<LP_OpenMeshImpl> &&mesh);

private:
    QString mFile;
    std::shared_ptr<LP_OpenMeshImpl> mEnt_Mesh;
    std::string mEntUid;    //Only set by Load()
};

#endif // LP_CMD_EXPORT_OBJ_H
