#ifndef LP_FUNCTIONALREGISTRY_H
#define LP_FUNCTIONALREGISTRY_H

#include <QByteArray>
#include <QAction>
#include <QDebug>

#include "Functional_global.h"
/*!
 * \class REGISTER_REACTION
 *
 * \brief It is suggested to use the following Operation Register for any inherit class of the base MyReaction class
 *		  such that every inherit class has an unique info. stored in m_Registered_ID.
 *
 * \author lip
 * \date Jun 2016
 */

#define REGISTER_FUNCTIONAL \
    static std::string mMenuName;\

#define REGISTER_FUNCTIONAL_IMPLEMENT(cls,a,x) \
    std::string cls::mMenuName(LP_FunctionalsRegistry::Register<cls>(#cls,#a,#x));\


class LP_Functional;

typedef LP_Functional *(*RegFuncPtr)(LP_Functional*);
typedef QMap<QByteArray, std::pair<QByteArray, QAction*>> FuncMap;

Q_DECLARE_METATYPE(RegFuncPtr)

class FUNCTIONAL_EXPORT LP_FunctionalsRegistry
{
    inline static FuncMap mReg = FuncMap();
public:
    inline static FuncMap*       Registry(){return &mReg;}

    template<typename T>
    inline static std::string Register( const std::string &cmd, const std::string &a, const std::string &m )
    {
        Q_ASSERT(!cmd.empty());
        Q_ASSERT(!a.empty());
        Q_ASSERT(!m.empty());
        QByteArray uid(cmd.c_str());
        QByteArray ba(a.c_str());
        QByteArray bm(m.c_str());
        auto pReg = Registry();
        bool rc = pReg->contains( uid );
        Q_ASSERT_X(!rc, "Functional registration", QString("ID \"%1\" has been registered before.").arg(cmd.c_str()).toLocal8Bit());

        auto action = new QAction(ba);
        action->setObjectName(uid.toLower());
        action->setData(QVariant::fromValue(&FC<T>));
        pReg->insert(uid, {bm, action});

        return m;
    }

    template<typename T>
    inline static LP_Functional *FC( LP_Functional* p = nullptr)
    {
        return new T( p );
    }
};

#endif // LP_FUNCTIONALREGISTRY_H
