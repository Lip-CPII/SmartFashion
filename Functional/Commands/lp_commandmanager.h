#ifndef LP_COMMANDMANAGER_H
#define LP_COMMANDMANAGER_H

#include "Functional_global.h"
#include <QUndoGroup>
#include <QUndoStack>
#include <QThread>

class LP_Command;
class LP_CommandStack;

class FUNCTIONAL_EXPORT LP_CommandGroup : public QUndoGroup
{
    Q_OBJECT
public:
    explicit LP_CommandGroup(QObject *parent = nullptr);
    inline LP_CommandStack *ActiveStack(){return qobject_cast<LP_CommandStack*>(activeStack());}
    static std::unique_ptr<LP_CommandGroup> gCommandGroup;

    // QObject interface
public:
    bool eventFilter(QObject *watched, QEvent *event) override;
};

class FUNCTIONAL_EXPORT LP_CommandStack : public QUndoStack//, std::enable_shared_from_this<LP_CommandStack>
{
    Q_OBJECT
public:
    explicit LP_CommandStack(QObject *parent = nullptr);
    virtual ~LP_CommandStack();
    void Push(LP_Command *cmd );
    bool Save(QDataStream &o);
    void Load(QDataStream &in);
    void Obsolete(LP_Command *cmd);
    static std::shared_ptr<LP_CommandStack> gCommands;  //TODO Multi stack

signals:
    void Pushed();

private:
    QThread mThread;
};



#endif // LP_COMMANDMANAGER_H
