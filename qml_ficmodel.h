#ifndef QML_FICMODEL_H
#define QML_FICMODEL_H

#include "UniversalModels/include/AdaptingTableModel.h"
#include <QQmlExtensionPlugin>

class FicModel : public AdaptingTableModel
{
    Q_OBJECT
public:
    enum FicRoles {
        AuthorRole = Qt::UserRole + 1,
        FandomRole,
        TitleRole,
        SummaryRole,
        GenreRole,
        UrlRole,
        TagsRole,
        OriginRole,
        LanguageRole,
        PublishedRole,
        UpdatedRole,
        CharactersRole,
        WordsRole,
        CompleteRole,
        CurrentChapterRole,
        ChaptersRole,
        ReviewsRole,
        FavesRole,
        RatedRole,
        AtChapterRole
    };

    QVariant data(const QModelIndex & index, int role) const;

    FicModel(QObject *parent = 0);
    QHash<int, QByteArray> roleNames() const;
    Q_INVOKABLE QVariantMap get(int idx) const;


    // QAbstractItemModel interface

};
//class MyModelPlugin : public QQmlExtensionPlugin
//{
//    Q_OBJECT
//    Q_PLUGIN_METADATA(IID "org.qt-project.QmlExtension.FicModel" FILE "ficmodel.json")
//public:
//    void registerTypes(const char *uri)
//    {
//        qmlRegisterType<FicModel>(uri, 1, 0,
//                "FicModel");
//    }
//};


#endif // QML_FICMODEL_H


