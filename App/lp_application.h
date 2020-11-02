#ifndef LP_APPLICATION_H
#define LP_APPLICATION_H

#include <QApplication>

class LP_Application : public QApplication
{
    Q_OBJECT
public:
    explicit LP_Application(int &argc, char **argv, int = ApplicationFlags);

signals:

    // QCoreApplication interface
public:
    bool notify(QObject *, QEvent *e) override;
};

#endif // LP_APPLICATION_H
