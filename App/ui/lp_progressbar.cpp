#include "lp_progressbar.h"
#include <QDebug>

LP_ProgressBar::LP_ProgressBar(QWidget *parent) : QProgressBar(parent)
{
    mAnim = std::make_unique<QPropertyAnimation>(this, "value");
    mAnim->setDuration(250);
}

void LP_ProgressBar::SetValue(const int &v)
{
    mNext = v;
 //   qDebug() << "End value : " << v;
    mAnim->setStartValue(value());
    mAnim->setEndValue(mNext);
    mAnim->start();
}

void LP_ProgressBar::ResetValue()
{
    mProgressing = false;
    mNext = LP_ProgressBar::gStep;
}
