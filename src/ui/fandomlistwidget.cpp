#include "ui/fandomlistwidget.h"
#include "ui_fandomlistwidget.h"

FandomListWidget::FandomListWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::FandomListWidget)
{
    ui->setupUi(this);
}

FandomListWidget::~FandomListWidget()
{
    delete ui;
}
