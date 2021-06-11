#ifndef LP_CMD_IMPORT_POINTCLOUD_H
#define LP_CMD_IMPORT_POINTCLOUD_H

#include "lp_command.h"
#include "lp_pointcloud.h"


class FUNCTIONAL_EXPORT LP_Cmd_Import_PointCloud : public LP_Command
{
    REGISTER(LP_Command, LP_Cmd_Import_PointCloud)
public:
    explicit LP_Cmd_Import_PointCloud(LP_Command *parent = nullptr);

    // QUndoCommand interface
    void undo() override;
    void redo() override;

    // LP_Command interface
    bool VerifyInputs() const override;
    bool Save(QDataStream &o) const override;
    bool Load(QDataStream &in) override;

    void SetFile(const QString &file);

private:
    QString mFile;
    std::shared_ptr<LP_PointCloudImpl> mEnt_PC;
    std::string mEntUid;    //Only set by Load()

};

#endif // LP_CMD_IMPORT_POINTCLOUD_H
