#ifndef LP_NEW_H
#define LP_NEW_H

#include "lp_functional.h"

class LP_New : public LP_Functional
{
    Q_OBJECT
    REGISTER_FUNCTIONAL
public:
    explicit LP_New(QObject *parent = nullptr);

    // LP_Functional interface
    bool Run() override;

signals:


};

#endif // LP_NEW_H
