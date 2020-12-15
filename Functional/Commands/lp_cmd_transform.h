#ifndef LP_GEOMETRY_TRANSFORM_H
#define LP_GEOMETRY_TRANSFORM_H

#include "Functional_global.h"
#include "Commands/lp_command.h"
#include <QMatrix4x4>


class FUNCTIONAL_EXPORT LP_Cmd_Transform : public LP_Command
{
    REGISTER(LP_Command, LP_Cmd_Transform)

public:
    explicit LP_Cmd_Transform(LP_Command *parent = nullptr);

    // QUndoCommand interface
public:
    void undo() override;
    void redo() override;

    // LP_Command interface
    bool VerifyInputs() const override;
    bool Save(QDataStream &o) const override;
    bool Load(QDataStream &in) override;

    void SetGeometry(QString uuid);
    void SetTrans(const QMatrix4x4 &trans );

private:
    QMatrix4x4 mTrans, mOrgTrans;
    QString mGeoUid;    //Only set by Load()
};

#endif // LP_GEOMETRY_TRANSFORM_H
