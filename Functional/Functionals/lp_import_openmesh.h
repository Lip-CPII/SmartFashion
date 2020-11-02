#ifndef LP_IMPORT_OPENMESH_H
#define LP_IMPORT_OPENMESH_H

#include "lp_functional.h"

class LP_Import_OpenMesh : public LP_Functional
{
    Q_OBJECT
    REGISTER_FUNCTIONAL
public:
    explicit LP_Import_OpenMesh(QObject *parent = nullptr);

    // LP_Functional interface
    QWidget *DockUi() override;
    bool Run() override;
signals:


};

#endif // LP_IMPORT_OPENMESH_H
