#ifndef TAGWIDGET_H
#define TAGWIDGET_H

#include <QWidget>


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

};

#endif // TAGWIDGET_H
