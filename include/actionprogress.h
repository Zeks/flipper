#ifndef ACTIONPROGRESS_H
#define ACTIONPROGRESS_H

#include <QWidget>

namespace Ui {
class ActionProgress;
}

class ActionProgress : public QWidget
{
    Q_OBJECT

public:
    explicit ActionProgress(QWidget *parent = 0);
    ~ActionProgress();
    Ui::ActionProgress *ui;
private:

};

#endif // ACTIONPROGRESS_H
