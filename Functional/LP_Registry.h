#ifndef LP_REGISTRY_H
#define LP_REGISTRY_H

#include <QByteArray>
#include <QAction>
#include <QDebug>
#include <QMap>


#include "Functional_global.h"
/*!
 * \class REGISTER
 *
 * \brief It is suggested to use the following Operation Register for any inherit class of the base MyReaction class
 *		  such that every inherit class has an unique info. stored in m_Registered_ID.
 *
 * \author lip
 * \date Jun 2016
 */



template <typename Base>
class FUNCTIONAL_EXPORT LP_Registry
{
    using _Func = std::function<Base *( Base *p )>;
    using RegMap = QMap<QByteArray, _Func>;

    template<typename Derived>
    static Base* _F(Base *p){return new Derived(p);}

public:
    inline static RegMap*       Registry(){
        static RegMap mReg;
        return &mReg;
    }

    template<typename Derived>
    inline static QByteArray Register( const std::string &cmd )
    {
        Q_ASSERT(!cmd.empty());
        QByteArray uid(cmd.c_str());
        auto pReg = Registry();
        bool rc = pReg->contains( uid );
        Q_ASSERT_X(!rc, "Registration", QString("ID \"%1\" has been registered before.").arg(cmd.c_str()).toLocal8Bit());

        pReg->insert(uid, &_F<Derived>);

        return QByteArray::fromStdString(cmd);
    }

private:
};


#define REGISTER(b,cls) \
    QByteArray ClassID() const override {return gClassName;} \
    inline static QByteArray gClassName = LP_Registry<b>::Register<cls>(#cls);\

#endif // LP_REGISTRY_H
