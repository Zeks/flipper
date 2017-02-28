#include "tagwidget.h"
#include "ui_tagwidget.h"
#include "genericeventfilter.h"
#include <QDebug>
#include <QMessageBox>

TagWidget::TagWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::TagWidget)
{
    ui->setupUi(this);
    ui->edtTags->setOpenLinks(false);
    connect(ui->edtTags, &QTextBrowser::anchorClicked, this, &TagWidget::OnTagClicked);
    ui->edtTags->setFont(QFont("Verdana", 12));


}

TagWidget::~TagWidget()
{
    delete ui;
}

void TagWidget::InitFromTags(int id, QList<QPair<QString, QString> > tags)
{
    currentId = id;
//    allTags.clear();
//    selectedTags.clear();
    ui->edtTags->clear();
    for(auto tag: tags)
    {
        QString toInsert ;
        if(tag.first == "1")
            toInsert = ("<a href=\"" + tag.first + " "  + tag.second + " \" style=\"color:darkgreen\" >" + tag.second + "</a>    ");
        else
            toInsert = ("<a href=\""  + tag.first + " " + tag.second + " \">" + tag.second + "</a>    ");
        allTags.push_back(tag.second);
        ui->edtTags->insertHtml(toInsert);
    }
}

QStringList TagWidget::GetSelectedTags()
{
    return selectedTags;
}

QStringList TagWidget::GetAllTags()
{
    return allTags;
}

void TagWidget::SetAddDialogVisibility(bool value)
{
    ui->wdgControls->setVisible(value);
}

void TagWidget::on_pbAddTag_clicked()
{
    QString tag = ui->leTag->text().trimmed();
    if(allTags.contains(tag))
    {
        QMessageBox::warning(nullptr, "Warning!", "Cannot add duplicate of a tag for sanity reasons." );
        return;
    }
    if(QRegExp("[^A-Za-z_-]").indexIn(tag) != -1)
    {
        QMessageBox::warning(nullptr, "Warning!", "Tags can only contain english letters and _ or - characters" );
        return;
    }
    allTags.push_back(tag);
    QString toInsert = "<a href=\"0 " + tag + " \">" + tag + "</a>    ";
    ui->edtTags->insertHtml(toInsert);
    emit tagAdded(ui->leTag->text());
}

void TagWidget::on_pbDeleteTag_clicked()
{
    QString tag = ui->leTag->text().trimmed();
    if(!allTags.contains(tag))
    {
        QMessageBox::warning(nullptr, "Warning!", "Cannot delete a tag that is not there." );
        return;
    }
    if(QRegExp("[^A-Za-z_-]").indexIn(tag) != -1)
    {
        QMessageBox::warning(nullptr, "Warning!", "Tags can only contain english letters and _ or - characters" );
        return;
    }
    OnRemoveTagFromEdit(tag);
    emit tagDeleted(ui->leTag->text());
}

void TagWidget::on_leTag_returnPressed()
{
    emit tagAdded(ui->leTag->text());
}

void TagWidget::OnTagClicked(const QUrl& url)
{
    QStringList temp = url.toString().split(" ");
    QString tag = temp.at(1);
    bool enabled = temp.at(0) == "1";
    bool doEnable = !enabled;

    QString text = ui->edtTags->toHtml();
    qDebug() << text;
    if(enabled)
    {
        text=text.replace("<a href=\"1 " +  tag + " \"><span style=\" text-decoration: underline; color:#006400;\">",
                     "<a href=\"0 "  + tag + " \"><span style=\" text-decoration: underline; color:#0000ff;\">");
        selectedTags.removeAll(tag);
    }
    else
    {
        text=text.replace("<a href=\"0 " + tag + " \"><span style=\" text-decoration: underline; color:#0000ff;\">",
                    "<a href=\"1 " + tag + " \"><span style=\" text-decoration: underline; color:#006400;\">");
        selectedTags.push_back(tag);
    }
    ui->edtTags->setHtml(text);


    emit tagToggled(currentId, tag, doEnable);
    emit refilter();
}

void TagWidget::OnNewTag(QString tag, bool enabled)
{
    QString text;
    if(enabled)
    {
        text = "<a href=\"1 " + tag + " \">" + tag  + "<span style=\" text-decoration: underline; color:#006400;\">";
        selectedTags.push_back(tag);
    }
    else
    {
        text = "<a href=\"0 " + tag + " \">" + tag + "</a>    ";
    }
    allTags.push_back(tag);
    QList<QPair<QString, QString>> pairs;
    for(auto tag : allTags)
    {
        if(selectedTags.contains(tag))
            pairs.push_back({"1", tag});
        else
            pairs.push_back({"0", tag});
    }
    InitFromTags(currentId, pairs);
}

void TagWidget::OnRemoveTagFromEdit(QString tag)
{
    QString text = ui->edtTags->toHtml();
    text=text.replace("<a href=\"1 " +  tag + " \"><span style=\" text-decoration: underline; color:#006400;\">" + tag + "</span></a>",
                 "");
    text=text.replace("<a href=\"0 " + tag + " \"><span style=\" text-decoration: underline; color:#0000ff;\">" + tag + "</span></a>",
                "");
    allTags.removeAll(tag);
    ui->edtTags->setHtml(text);
}

