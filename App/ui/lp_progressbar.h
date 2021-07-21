#ifndef LP_PROGRESSBAR_H
#define LP_PROGRESSBAR_H

#include <QProgressBar>
#include <QPropertyAnimation>
#include <memory>

class LP_ProgressBar : public QProgressBar
{
    Q_OBJECT
public:
    explicit LP_ProgressBar(QWidget *parent = nullptr);

    void SetValue(const int &v);
    inline int NextValue() const {return mNext;}
    void ResetValue();
    bool IsProgress() const {return mProgressing;};
    void SetProgressing(bool b=true) { mProgressing = b;}

    constexpr static int gStep = 20;
signals:

private:
    bool mProgressing = false;
    int mNext;
    std::unique_ptr<QPropertyAnimation> mAnim;
};

#endif // LP_PROGRESSBAR_H
