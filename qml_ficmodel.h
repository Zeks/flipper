/*
FFSSE is a replacement search engine for fanfiction.net search results
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
        AtChapterRole,
        RownumRole,
        RecommendationsRole,
        FicIdRole,
        EndRole = RecommendationsRole

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


