#ifndef LP_COMMAND_H
#define LP_COMMAND_H

#include <QUndoCommand>
#include "LP_Registry.h"



class LP_Command : public QUndoCommand
{
public:
    explicit LP_Command(LP_Command *parent = nullptr) : QUndoCommand(parent),mStatus(-std::numeric_limits<int>::max()){}
    virtual bool VerifyInputs() const {return false;}
    virtual bool Save(QDataStream &) const {Q_ASSERT_X(0, __FILE__, "Please override the Save()");return false;}
    virtual bool Load(QDataStream &){Q_ASSERT_X(0, __FILE__, "Please override the Load()");return false;}
    virtual QByteArray ClassID() const = 0;
protected:
    int mStatus;
};

#endif // LP_COMMAND_H
