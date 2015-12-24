#include "fanficdisplay.h"
#include "ui_fanficdisplay.h"

FanficDisplay::FanficDisplay(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::FanficDisplay)
{
    ui->setupUi(this);
}

FanficDisplay::~FanficDisplay()
{
    delete ui;
}
