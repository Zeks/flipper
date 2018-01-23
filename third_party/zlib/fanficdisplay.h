#ifndef FANFICDISPLAY_H
#define FANFICDISPLAY_H

#include <QWidget>

namespace Ui {
class FanficDisplay;
}

class FanficDisplay : public QWidget
{
    Q_OBJECT

public:
    explicit FanficDisplay(QWidget *parent = 0);
    ~FanficDisplay();

private:
    Ui::FanficDisplay *ui;
};

#endif // FANFICDISPLAY_H
