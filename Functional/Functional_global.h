#ifndef FUNCTIONAL_GLOBAL_H
#define FUNCTIONAL_GLOBAL_H

#include <QtCore/qglobal.h>

#if defined(FUNCTIONAL_LIBRARY)
#  define FUNCTIONAL_EXPORT Q_DECL_EXPORT
#else
#  define FUNCTIONAL_EXPORT Q_DECL_IMPORT
#endif

#endif // FUNCTIONAL_GLOBAL_H
