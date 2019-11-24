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
#include "qml_ficmodel.h"
#include "core/fic_genre_data.h"
#include <QDebug>
#include <QDateTime>

QVariant FicModel::data(const QModelIndex &index, int role) const
{
    //<< "fandom" << "author" << "title" << "summary" << "genre" << "characters" << "rated" <<
    //"published" << "updated" << "url" << "tags" << "wordCount" << "favourites" << "reviews" << "chapters" << "complete" << "atChapter" << rownum);
    if(index.column() == 0  && ( role > Qt::UserRole && role < EndRole + 1))
    {
        if(role == FandomRole)
            return AdaptingTableModel::data(index.sibling(index.row(), 0), Qt::DisplayRole).toString().replace("CROSSOVER", "");
        if(role == AuthorRole)
            return AdaptingTableModel::data(index.sibling(index.row(), 1), Qt::DisplayRole);
        if(role == TitleRole)
            return AdaptingTableModel::data(index.sibling(index.row(), 2), Qt::DisplayRole);
        if(role == SummaryRole)
            return AdaptingTableModel::data(index.sibling(index.row(), 3), Qt::DisplayRole);
//        if(role == GenreRole)
//            return AdaptingTableModel::data(index.sibling(index.row(), 4), Qt::DisplayRole).toString().split("/");
        if(role == GenreRole)
            return AdaptingTableModel::data(index.sibling(index.row(), 4), Qt::DisplayRole).toString().split("/");
        if(role == CharactersRole)
            return AdaptingTableModel::data(index.sibling(index.row(), 5), Qt::DisplayRole).toString();
        if(role == RatedRole)
            return AdaptingTableModel::data(index.sibling(index.row(), 6), Qt::DisplayRole);
        if(role == PublishedRole)
            return AdaptingTableModel::data(index.sibling(index.row(), 7), Qt::DisplayRole).toDateTime();
        if(role == UpdatedRole)
            return AdaptingTableModel::data(index.sibling(index.row(), 8), Qt::DisplayRole).toDateTime();
        if(role == UrlRole)
            return AdaptingTableModel::data(index.sibling(index.row(), 9), Qt::DisplayRole);
        if(role == TagsRole)
        {
            QStringList temp = AdaptingTableModel::data(index.sibling(index.row(), 10), Qt::DisplayRole).toString().replace("none", "").trimmed().split(" ", QString::SkipEmptyParts);
            return temp;
        }
        if(role == WordsRole)
            return AdaptingTableModel::data(index.sibling(index.row(), 11), Qt::DisplayRole).toString();
        if(role == FavesRole)
            return AdaptingTableModel::data(index.sibling(index.row(), 12), Qt::DisplayRole);
        if(role == ReviewsRole)
            return AdaptingTableModel::data(index.sibling(index.row(), 13), Qt::DisplayRole);
        if(role == ChaptersRole)
            return AdaptingTableModel::data(index.sibling(index.row(), 14), Qt::DisplayRole).toInt();
        if(role == CompleteRole)
            return AdaptingTableModel::data(index.sibling(index.row(), 15), Qt::DisplayRole).toInt();
        if(role == OriginRole)
            return QString("");
        if(role == AtChapterRole)
            return AdaptingTableModel::data(index.sibling(index.row(), 16), Qt::DisplayRole).toInt();
        if(role == RownumRole)
            return AdaptingTableModel::data(index.sibling(index.row(), 17), Qt::DisplayRole).toInt();
        if(role == RecommendationsMainRole)
            return AdaptingTableModel::data(index.sibling(index.row(), 18), Qt::DisplayRole).toInt();
        if(role == LanguageRole)
            return QString("");
        if(role == RealGenreRole)
            return AdaptingTableModel::data(index.sibling(index.row(), 19), Qt::DisplayRole).toString().split(",", QString::SkipEmptyParts);
        if(role == AuthorIdRole)
        {
            auto value = AdaptingTableModel::data(index.sibling(index.row(), 20), Qt::DisplayRole).toInt();
            return AdaptingTableModel::data(index.sibling(index.row(), 20), Qt::DisplayRole).toInt();
        }
        if(role == SlashRole)
            return AdaptingTableModel::data(index.sibling(index.row(), 21), Qt::DisplayRole).toInt();
        if(role == BreakdownRole)
        {
            auto var = AdaptingTableModel::data(index.sibling(index.row(), 22), Qt::DisplayRole);
            return var;
        }
        if(role == BreakdownCountRole)
        {
            auto var = AdaptingTableModel::data(index.sibling(index.row(), 23), Qt::DisplayRole);
            return var;
        }
        if(role == LikedAuthorRole)
        {
            auto var = AdaptingTableModel::data(index.sibling(index.row(), 24), Qt::DisplayRole);
            return var;
        }
        if(role == PurgedRole)
        {
            auto var = AdaptingTableModel::data(index.sibling(index.row(), 25), Qt::DisplayRole);
            return var;
        }
        if(role == ScoreRole)
        {
            auto var = AdaptingTableModel::data(index.sibling(index.row(), 26), Qt::DisplayRole);
            return var;
        }
        if(role == SnoozeExpiredRole)
        {
            auto var = AdaptingTableModel::data(index.sibling(index.row(), 27), Qt::DisplayRole);
            return var;
        }
        if(role == SnoozeModeRole)
            return AdaptingTableModel::data(index.sibling(index.row(), 28), Qt::DisplayRole).toInt();
        if(role == SnoozeLimitRole)
            return AdaptingTableModel::data(index.sibling(index.row(), 29), Qt::DisplayRole).toInt();
        if(role == SnoozeOriginRole)
            return AdaptingTableModel::data(index.sibling(index.row(), 30), Qt::DisplayRole).toInt();


        if(role == NotesRole)
        {
            auto var = AdaptingTableModel::data(index.sibling(index.row(), 31), Qt::DisplayRole);
            return var;
        }
        if(role == QuotesRole)
        {
            auto var = AdaptingTableModel::data(index.sibling(index.row(), 32), Qt::DisplayRole);
            return var;
        }
        if(role == SelectedRole)
        {
            auto var = AdaptingTableModel::data(index.sibling(index.row(), 33), Qt::DisplayRole);
            return var;
        }
        if(role == RecommendationsSecondRole)
            return AdaptingTableModel::data(index.sibling(index.row(), 34), Qt::DisplayRole).toInt();
        if(role == PlaceInMainList)
            return AdaptingTableModel::data(index.sibling(index.row(), 35), Qt::DisplayRole).toInt();
        if(role == PlaceInSecondList)
            return AdaptingTableModel::data(index.sibling(index.row(), 36), Qt::DisplayRole).toInt();
        if(role == PlaceOnFirstPedestal)
            return AdaptingTableModel::data(index.sibling(index.row(), 37), Qt::DisplayRole).toInt();
        if(role == PlaceOnSecondPedestal)
            return AdaptingTableModel::data(index.sibling(index.row(), 38), Qt::DisplayRole).toInt();

        if(role == CurrentChapterRole)
            return QString("");
        {
            qDebug() << index.row() << "  " << static_cast<int>(Qt::UserRole) << " " << index.column() << " " << "returning empty: " << role;
            return QString("");
        }

    }
    return AdaptingTableModel::data(index, role);

}


FicModel::FicModel(QObject *parent) : AdaptingTableModel(parent)
{

}


QHash<int, QByteArray> FicModel::roleNames() const {
    QHash<int, QByteArray> roles;
    roles[AuthorRole] = "author";
    roles[AuthorIdRole] = "author_id";
    roles[FandomRole] = "fandom";
    roles[TitleRole] = "title";
    roles[SummaryRole] = "summary";
    roles[GenreRole] = "genre";
    roles[UrlRole] = "url";
    roles[TagsRole] = "tags";
    roles[OriginRole] = "origin";
    roles[LanguageRole] = "language";
    roles[PublishedRole] = "published";
    roles[UpdatedRole] = "updated";
    roles[CharactersRole] = "characters";
    roles[WordsRole] = "words";
    roles[CompleteRole] = "complete";
    roles[CurrentChapterRole] = "currentchapter";
    roles[ChaptersRole] = "chapters";
    roles[ReviewsRole] = "reviews";
    roles[FavesRole] = "favourites";
    roles[RatedRole] = "rated";
    roles[AtChapterRole] = "atChapter";
    roles[RownumRole] = "rownum";
    roles[FicIdRole] = "ID";
    roles[RecommendationsMainRole] = "recommendationsMain";
    roles[RealGenreRole] = "realGenre";
    roles[SlashRole] = "minSlashLevel";
    roles[BreakdownRole] = "roleBreakdown";
    roles[BreakdownCountRole] = "roleBreakdownCount";
    roles[LikedAuthorRole] = "likedAuthor";
    roles[PurgedRole] = "purged";
    roles[ScoreRole] = "score";
    roles[SnoozeExpiredRole] = "snoozeExpired";
    roles[SnoozeModeRole] = "snoozeMode";
    roles[SnoozeLimitRole] = "snoozeLimit";
    roles[SnoozeOriginRole] = "snoozeOrigin";
    roles[NotesRole] = "notes";
    roles[QuotesRole] = "quotes";
    roles[SelectedRole] = "selected";
    roles[RecommendationsSecondRole] = "recommendationsSecond";
    roles[PlaceInMainList] = "placeMain";
    roles[PlaceInSecondList] = "placeSecond";
    roles[PlaceOnFirstPedestal] = "placeOnFirstPedestal";
    roles[PlaceOnSecondPedestal] = "placeOnSecondPedestal";

    return roles;
}

QVariantMap FicModel::get(int idx) const {
  QVariantMap map;
  foreach(int k, roleNames().keys()) {
    map[roleNames().value(k)] = data(AdaptingTableModel::index(idx, 0), k);
  }
  return map;
}
