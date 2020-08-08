#pragma once
#include "core/db_entity.h"
#include "core/fic_genre_data.h"
#include "core/identity.h"
#include "core/slash_data.h"

#include <QSharedPointer>
#include <QDateTime>


namespace core {
class Author;
class Fic;
typedef QSharedPointer<Fic> FicPtr;

class Fic : public DBEntity{
public:
    enum EFicSource{
        efs_search = 0,
        efs_favourites = 1,
        efs_own_works = 2
    };
    enum EFicSnoozeMode{
        efsm_next_chapter = 0,
        efsm_target_chapter= 1,
        efsm_til_finished = 2
    };
    struct Statistics
    {
    public:
        void Log();
        bool isValid = false;

        int age;
        int daysRunning;
        int minSlashPass = 0;

        double wcr;
        double wcr_adjusted;
        double reviewsTofavourites;

        QString realGenreString;



        QList<genre_stats::GenreBit> realGenreData;
    };

    struct UserData{
        bool likedAuthor = false;
        bool ficIsSnoozed = false;
        bool snoozeExpired = false;
        EFicSnoozeMode snoozeMode = EFicSnoozeMode::efsm_next_chapter;
        int atChapter=0;
        int chapterTillSnoozed = -1;
        int chapterSnoozed = -1;
        QString tags;
    };

    struct RecommendationListData{
        bool purged = false;
        int recommendationsMainList = 0;
        int recommendationsSecondList = 0;
        int placeInMainList = 0;
        int placeInSecondList = 0;
        int placeOnFirstPedestal= 0;
        int placeOnSecondPedestal = 0;
        QStringList voteBreakdown;
        QStringList voteBreakdownCounts;
    };

    struct UiData{
        bool selected = false;
    };

    Fic();
    Fic(const Fic&) = default;
    Fic& operator=(const Fic&) = default;
    ~Fic(){}
    void Log();
    void LogUrls();
    void LogWebIds();
    static FicPtr NewFanfic() { return QSharedPointer<Fic>(new Fic);}

    bool isCrossover = false;
    bool isValid =false;
    int complete=0;
    int score = 0;
    int author_id = -1;

    QString urlFFN;
    QString webSite = "ffn";

    QString wordCount;
    QString chapters;
    QString reviews;
    QString favourites;
    QString follows;
    QString rated;
    QString fandom;
    QStringList fandoms;
    QString title;
    QString language;
    QString genreString;
    QString summary;
    QString statSection;
    QString notes;

    QString charactersFull;
    QString authorName;

    QSharedPointer<Author> author;


    QStringList genres;
    QStringList characters;
    QStringList quotes;

    QHash<QString, QString> urls;

    QDateTime published;
    QDateTime updated;


    QList<int> fandomIds;

    UpdateMode updateMode = UpdateMode::none;
    EFicSource ficSource = efs_search;
    UserData userData;
    UiData uiData;
    RecommendationListData recommendationsData;
    SlashData slashData;
    Statistics statistics;
    Identity identity;

    void SetGenres(QString genreString, QString website){

        this->genreString = genreString;
        QStringList genresList;
        bool hasHurt = false;
        QString fixedGenreString = genreString;
        if(website == "ffn" && genreString.contains("Hurt/Comfort"))
        {
            hasHurt = true;
            fixedGenreString = fixedGenreString.replace("Hurt/Comfort", "");
        }

        if(website == "ffn")
            genresList = fixedGenreString.split("/", QString::SkipEmptyParts);
        if(hasHurt)
            genresList.push_back("Hurt/Comfort");
        for(auto& genre: genresList)
            genre = genre.trimmed();
        genres = genresList;


    }
    QString url(QString type)
    {
        if(urls.contains(type))
            return urls[type];
        return "";
    }
    void SetUrl(QString type, QString url);


    Statistics getCalcStats() const;
    void setCalcStats(const Statistics &value);
    int GetIdInDatabase() const { return identity.id; }
};

}
