/*
Flipper is a replacement search engine for fanfiction.net search results
Copyright (C) 2017-2019  Marchenko Nikolai

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>
*/
#include "tagwidget.h"
#include "ui_tagwidget.h"
#include "Interfaces/fandoms.h"
#include "Interfaces/tags.h"
#include "genericeventfilter.h"
#include "pure_sql.h"
#include <QDebug>
#include <QMessageBox>
#include <QStringListModel>
#include <QSettings>
#include <QTextCodec>

TagWidget::TagWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::TagWidget)
{
    ui->setupUi(this);
    ui->edtTags->setOpenLinks(false);
    connect(ui->edtTags, &QTextBrowser::anchorClicked, this, &TagWidget::OnTagClicked);
    connect(ui->pbTagExport, &QPushButton::clicked, this, &TagWidget::OnTagExport);
    connect(ui->pbTagImport, &QPushButton::clicked, this, &TagWidget::OnTagImport);
    ui->edtTags->setFont(QFont("Verdana", 12));

    QSettings settings("settings/settings.ini", QSettings::IniFormat);
    settings.setIniCodec(QTextCodec::codecForName("UTF-8"));
    ui->wdgMassTag->setVisible(settings.value("Settings/showMassTagger", false).toBool());


}

TagWidget::~TagWidget()
{
    delete ui;
}

void TagWidget::InitFromTags(int id, QList<QPair<QString, QString> > tags)
{
    currentId = id;
    QHash<QString, int> tagSizes;
    QStringList tagList;
    for(auto pair : tags)
        tagList.push_back(pair.second);
    if(ui->chkDisplayTagSize->isChecked())
        tagSizes = tagsInterface->GetTagSizes(tagList);


    ui->edtTags->clear();
    for(auto tag: tags)
    {
        QString toInsert ;
        QString label;
        if(ui->chkDisplayTagSize->isChecked())
            label = tag.second + "(" + QString::number(tagSizes[tag.second]) +  ")";
        else
            label = tag.second;

        if(tag.first == "1")
            toInsert = ("<a href=\"" + tag.first + " "  + tag.second + " \" style=\"color:darkgreen\" >" + label + "</a>    ");
        else
            toInsert = ("<a href=\""  + tag.first + " " + tag.second + " \">" + label + "</a>    ");
        allTags.push_back(tag.second);
        ui->edtTags->insertHtml(toInsert);
//        if(ui->chkDisplayTagSize->isChecked())
//            ui->edtTags->insertHtml("<br>");
    }
    ui->cbAssignTag->setModel(new QStringListModel(allTags));
    ui->cbFandom->setModel(new QStringListModel(fandomsInterface->GetFandomList()));
}

void TagWidget::InitEditFromTags(QStringList tags)
{
    QHash<QString, int> tagSizes;
    if(ui->chkDisplayTagSize)
        tagSizes = tagsInterface->GetTagSizes(tags);

    ui->edtTags->clear();
    for(auto tag: tags)
    {
        QString label;
        if(ui->chkDisplayTagSize->isChecked())
            label = tag + "(" + QString::number(tagSizes[tag]) +  ")";
        else
            label = tag;

        auto toInsert = ("<a href=\"0 " + tag + " \">" + label + "</a>    ");
        ui->edtTags->insertHtml(toInsert);
//        if(ui->chkDisplayTagSize->isChecked())
//            ui->edtTags->insertHtml("<br>");
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

void TagWidget::SetAddDialogVisibility(bool )
{
    //ui->wdgControls->setVisible(value);
}

bool TagWidget::UseTagsForAuthors()
{
    return ui->chkUseTagsForAuthors->isChecked();
}

void TagWidget::SetTagsForAuthorsMode(bool value)
{
    ui->chkUseTagsForAuthors->setChecked(value);
}

bool TagWidget::UseANDForTags()
{
    return ui->chkAndForTags->isChecked();
}

void TagWidget::SetUseANDForTags(bool value)
{
    ui->chkAndForTags->setChecked(value);
}

void TagWidget::ClearSelection()
{
    InitEditFromTags(allTags);
}

bool TagWidget::DbIdsRequested()
{
    return ui->cbTagEntitySelector->currentIndex() == 2;
}

void TagWidget::ResetFilters()
{
    ClearSelection();
    ui->chkAndForTags->setChecked(false) ;
    ui->chkUseTagsForAuthors->setChecked(false);
    ui->chkTagIncludingCrosses->setChecked(false);
    selectedTags.clear();
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
    ui->cbAssignTag->setModel(new QStringListModel(allTags));
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
    ui->cbAssignTag->setModel(new QStringListModel(allTags));
    emit tagDeleted(ui->leTag->text());
}

void TagWidget::on_leTag_returnPressed()
{
    emit tagAdded(ui->leTag->text());
}

void TagWidget::SelectTags(QStringList tags)
{
    QString text = ui->edtTags->toHtml();
    for(auto tag: tags)
    {
        text=text.replace("<a href=\"0 " + tag + " \"><span style=\" text-decoration: underline; color:#0000ff;\">",
                    "<a href=\"1 " + tag + " \"><span style=\" text-decoration: underline; color:#006400;\">");
        selectedTags.push_back(tag);
    }
    ui->edtTags->setHtml(text);
}

void TagWidget::ClearAuthorsForTags()
{
    ui->chkUseTagsForAuthors->setChecked(false);
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


    emit tagToggled(tag, doEnable);
    //emit refilter();
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

void TagWidget::OnTagExport()
{
    QMessageBox m;
    m.setIcon(QMessageBox::Warning);
    m.setText("Cliclking Yes will export tags to Tags.sqlite.");
    m.addButton("Yes",QMessageBox::AcceptRole);
    auto dropTask = m.addButton("No",QMessageBox::AcceptRole);
    m.exec();
    if(m.clickedButton() == dropTask)
        return;
    tagsInterface->ExportToFile("TagExport.sqlite");
}

void TagWidget::OnTagImport()
{
    QMessageBox m;
    m.setIcon(QMessageBox::Warning);
    m.setText("Cliclking Yes will import tags from Tags.sqlite into your database.");
    m.addButton("Yes",QMessageBox::AcceptRole);
    auto dropTask = m.addButton("No",QMessageBox::AcceptRole);
    m.exec();
    if(m.clickedButton() == dropTask)
        return;
    tagsInterface->ImportFromFile("TagExport.sqlite");
    emit dbIDRequest();
    emit tagReloadRequested();
}


void TagWidget::on_pbAssignTagToFandom_clicked()
{
    fandomsInterface->AssignTagToFandom(ui->cbFandom->currentText(),
                                        ui->cbAssignTag->currentText(),
                                        ui->chkTagIncludingCrosses->isChecked());
}


void TagWidget::on_pbUrlsForTags_clicked()
{
    emit createUrlsForTags(ui->cbTagEntitySelector->currentIndex() == 1);
}

void TagWidget::on_chkDisplayTagSize_stateChanged(int)
{
    InitEditFromTags(allTags);
    SelectTags(selectedTags);
}

void TagWidget::on_pbPurgeSelectedTags_clicked()
{
    QMessageBox::StandardButton reply;
     reply = QMessageBox::question(this, "Warning!", "This will remove selected tags from every fic. Do you want to continue?",
                                   QMessageBox::Yes|QMessageBox::No);
     if (reply != QMessageBox::Yes)
         return;
     tagsInterface->RemoveTagsFromEveryFic(GetSelectedTags());
     InitEditFromTags(allTags);
     SelectTags(selectedTags);
}

void TagWidget::on_chkUseTagsForAuthors_stateChanged(int)
{
    if(ui->chkUseTagsForAuthors->isChecked())
        emit clearLikedAuthors();
}

void TagWidget::on_pbTagFromClipboard_clicked()
{
    emit tagFromClipboard();
}
