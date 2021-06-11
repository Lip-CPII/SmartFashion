#ifndef LP_EXPORT_OBJ_H
#define LP_EXPORT_OBJ_H

#include "lp_functional.h"

class LP_Export_Obj : public LP_Functional
{
    Q_OBJECT
    REGISTER_FUNCTIONAL
public:
    explicit LP_Export_Obj(QObject *parent = nullptr);

    // LP_Functional interface
    QWidget *DockUi() override;
    bool Run() override;
signals:


};

#endif // LP_EXPORT_OBJ_H
