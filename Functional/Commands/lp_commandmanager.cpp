#include "lp_commandmanager.h"
#include "lp_command.h"

#include <QDataStream>
#include <QDebug>
#include <QApplication>

std::shared_ptr<LP_CommandStack> LP_CommandStack::gCommands = std::make_shared<LP_CommandStack>();
std::unique_ptr<LP_CommandGroup> LP_CommandGroup::gCommandGroup = std::make_unique<LP_CommandGroup>();


LP_CommandGroup::LP_CommandGroup(QObject *parent) : QUndoGroup(parent)
{
    addStack(LP_CommandStack::gCommands.get());
}

LP_CommandStack::LP_CommandStack(QObject *parent) : QUndoStack(parent)
{
    moveToThread(&mThread);
    mThread.start();
}

LP_CommandStack::~LP_CommandStack()
{
    mThread.quit();
    mThread.wait(5000);

    mThread.terminate();
}

void LP_CommandStack::Push(LP_Command *cmd)
{
    QApplication::setOverrideCursor(Qt::WaitCursor);
    qApp->installEventFilter(LP_CommandGroup::gCommandGroup.get());

    QMetaObject::invokeMethod((QUndoStack*)this, [cmd,this](){
        push(cmd);
    }, Qt::QueuedConnection);
}

bool LP_CommandStack::Save(QDataStream &o)
{
    int nCmd = count();
    o << nCmd;
    bool rc = 1;
    for ( auto i=0; i<nCmd; ++i ){
        auto cmd = static_cast<const LP_Command*>(command(i));
        o << cmd->ClassID();
        rc &= cmd->Save(o);
        qDebug() << "Saving (" << cmd->text() << ") : " << rc;
    }
    return rc;
}

void LP_CommandStack::Load(QDataStream &in)
{
    clear();    //Clean up the stack
    auto reg = LP_Registry<LP_Command>::Registry();
    auto regEnd = reg->cend();
    int nCmd{0};
    in >> nCmd;

    qDebug() << nCmd;

    for ( auto i=0; i<nCmd; ++i ){
        QByteArray id;
        in >> id;
        auto it = reg->find(id);
        if ( regEnd == it ){
            throw std::runtime_error("Unknow command or invalid file");
        }
        auto cmd = it.value()(nullptr);
        if ( cmd->Load(in)){
            Push(cmd);
        }
    }
}


bool LP_CommandGroup::eventFilter(QObject *watched, QEvent *event)
{
    if ( event->type() == QEvent::KeyPress ||
         event->type() == QEvent::KeyRelease ||
         event->type() == QEvent::MouseButtonRelease ||
         event->type() == QEvent::MouseButtonPress ||
         event->type() == QEvent::MouseButtonDblClick ||
         event->type() == QEvent::MouseMove ||
         event->type() == QEvent::Close )
    {
        if ( watched->objectName().contains("LP_MainWindow") ||
             watched->objectName().contains("openGLWidget")){
            return QObject::eventFilter( watched, event );
        }
        return true;
    }
    return QObject::eventFilter( watched, event );
}
