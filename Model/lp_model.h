#ifndef LP_MODEL_H
#define LP_MODEL_H

#include "Model_global.h"
#include <QStandardItemModel>

class MODEL_EXPORT LP_Model : public QStandardItemModel
{
    Q_OBJECT
public:
    explicit LP_Model();
    virtual ~LP_Model();

    static LP_Model g_Model;
};

#endif // LP_MODEL_H
