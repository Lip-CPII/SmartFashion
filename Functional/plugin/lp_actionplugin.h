#ifndef LP_ACTIONPLUGIN_H
#define LP_ACTIONPLUGIN_H

#include <QtPlugin>
#include "Functional_global.h"

#include "Functionals/lp_functional.h"

#define LP_ActionPlugin_iid "cpii.rp5.SmartFashion.ActionPlugin.Interface/0.1"


class QAction;

/**
 * @brief The LP_ActionPlugin class
 * Plugin interface from Functional
 * Run() must be implemented as overrided from Functional
 * In addition, MenuName() and Trigger() must be implemented
 */
class FUNCTIONAL_EXPORT LP_ActionPlugin : public LP_Functional
{
    Q_OBJECT
public:
    explicit LP_ActionPlugin(QObject *parent = nullptr):LP_Functional(parent),mAction(nullptr){}

    /**
     * @brief MenuName
     * @return The name of the menu wanted to be placed in the Main-GUI menubar
     * Remark: name of the menu must begin with "menu", e.g. menuPlugin
     */
    virtual QString MenuName() = 0;

    /**
     * @brief Trigger
     * @return The action triggers this plugin. THe trigger name should be provided when
     * creating the QAction, e.g. mAction.
     */
    virtual QAction *Trigger() = 0;

protected:
    QAction *mAction;
};

Q_DECLARE_INTERFACE(LP_ActionPlugin, LP_ActionPlugin_iid)

#endif // LP_ACTIONPLUGIN_H
