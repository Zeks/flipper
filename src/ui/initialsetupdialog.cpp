#include "include/ui/initialsetupdialog.h"
#include "ui_initialsetupdialog.h"

InitialSetupDialog::InitialSetupDialog(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::InitialSetupDialog)
{
    ui->setupUi(this);
}

InitialSetupDialog::~InitialSetupDialog()
{
    delete ui;
}
