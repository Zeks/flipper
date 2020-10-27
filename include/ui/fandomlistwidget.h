#ifndef FANFICLISTWIDGET_H
#define FANFICLISTWIDGET_H

#include <QWidget>

namespace Ui {
class FandomListWidget;
}

class FandomListWidget : public QWidget
{
    Q_OBJECT

public:
    explicit FandomListWidget(QWidget *parent = nullptr);
    ~FandomListWidget();

private:
    Ui::FandomListWidget *ui;
};

#endif // FANFICLISTWIDGET_H
