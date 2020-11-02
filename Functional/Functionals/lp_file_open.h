#ifndef LP_FILE_OPEN_H
#define LP_FILE_OPEN_H


#include "lp_functional.h"

class LP_File_Open : public LP_Functional
{
    Q_OBJECT
    REGISTER_FUNCTIONAL
public:
    explicit LP_File_Open(QObject *parent = nullptr);

signals:

    // LP_Functional interface
public:
    bool Run() override;
};

#endif // LP_FILE_OPEN_H
