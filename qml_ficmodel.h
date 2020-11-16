/*
Flipper is a recommendation and search engine for fanfiction.net
Copyright (C) 2017-2020  Marchenko Nikolai

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
#pragma once
#include "ui-models/include/AdaptingTableModel.h"
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
        RecommendationsMainRole,
        RealGenreRole,
        FicIdRole,
        AuthorIdRole,
        SlashRole,
        BreakdownRole,
        BreakdownCountRole,
        LikedAuthorRole,
        PurgedRole,
        ScoreRole,
        SnoozeExpiredRole,
        SnoozeModeRole,
        SnoozeLimitRole,
        SnoozeOriginRole,
        NotesRole,
        QuotesRole,
        SelectedRole,
        RecommendationsSecondRole,
        PlaceInMainList,
        PlaceInSecondList,
        PlaceOnFirstPedestal,
        PlaceOnSecondPedestal,
        FicIsSnoozed,
        EndRole = FicIsSnoozed,
    };

    QVariant data(const QModelIndex & index, int role) const;

    FicModel(QObject *parent = nullptr);
    QHash<int, QByteArray> roleNames() const;
    Q_INVOKABLE QVariantMap get(int idx) const;


    // QAbstractItemModel interface

};

