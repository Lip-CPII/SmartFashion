#include "lp_application.h"
#include <QDebug>

LP_Application::LP_Application(int &argc, char **argv, int flags) : QApplication(argc, argv, flags)
{

}

bool LP_Application::notify(QObject *o, QEvent *e)
{
    bool done = true;
      try {
        done = QApplication::notify(o, e);
      } catch (const std::exception& ex) {
        qWarning() << "Exception :" << ex.what();
      } catch (...) {
        qCritical() << "Unkown failure : " << o << "-" << e;
      }
      return done;
}
