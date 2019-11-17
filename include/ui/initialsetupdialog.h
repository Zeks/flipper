#ifndef INITIALSETUPDIALOG_H
#define INITIALSETUPDIALOG_H

#include <QWidget>

namespace Ui {
class InitialSetupDialog;
}

class InitialSetupDialog : public QWidget
{
    Q_OBJECT

public:
    explicit InitialSetupDialog(QWidget *parent = nullptr);
    ~InitialSetupDialog();

private:
    Ui::InitialSetupDialog *ui;
};

#endif // INITIALSETUPDIALOG_H
