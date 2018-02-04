/*
Flipper is a replacement search engine for fanfiction.net search results
Copyright (C) 2017  Marchenko Nikolai

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
#ifndef TAGWIDGET_H
#define TAGWIDGET_H

#include <QWidget>
namespace interfaces{
class Fandoms;
class Tags;
}

namespace Ui {
class TagWidget;
}

class TagWidget : public QWidget
{
    Q_OBJECT

public:
    explicit TagWidget(QWidget *parent = 0);
    ~TagWidget();
    void InitFromTags(int currentId, QList<QPair<QString, QString>>);
    QStringList GetSelectedTags();
    QStringList GetAllTags();
    void SetAddDialogVisibility(bool);
    QSharedPointer<interfaces::Fandoms> fandomsInterface;
    QSharedPointer<interfaces::Tags> tagsInterface;
private:
    Ui::TagWidget *ui;
    int currentId;
    QStringList selectedTags;
    QStringList allTags;



signals:
    void tagToggled(int, QString, bool);
    void tagAdded(QString);
    void tagDeleted(QString);
    void refilter();


public slots:
    void on_pbAddTag_clicked();
    void on_pbDeleteTag_clicked();
    void on_leTag_returnPressed();
    void OnTagClicked(const QUrl &);
    void OnNewTag(QString, bool);
    void OnRemoveTagFromEdit(QString);
    void OnTagExport();
    void OnTagImport();

private slots:
    void on_pbAssignTagToFandom_clicked();
};

#endif // TAGWIDGET_H
