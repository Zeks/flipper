#pragma once
#include "core/db_entity.h"
#include "core/fic_genre_data.h"
#include "core/identity.h"
#include "core/slash_data.h"

#include <QSharedPointer>
#include <QDateTime>


namespace core {
class Author;
class Fanfic;
typedef QSharedPointer<Fanfic> FicPtr;

class Fanfic : public DBEntity{
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

    Fanfic();
    Fanfic(const Fanfic&) = default;
    Fanfic& operator=(const Fanfic&) = default;
    ~Fanfic(){}
    void Log();
    void LogUrls();
    void LogWebIds();
    static FicPtr NewFanfic() { return QSharedPointer<Fanfic>(new Fanfic);}

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
            genresList = fixedGenreString.split("/", Qt::SkipEmptyParts);
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



struct FanficDataForRecommendationCreation
{
    bool complete = false;
    bool slash = false;
    bool dead = false;
    bool sameLanguage = true;
    bool adult = false;

    int id = -1;
    int sumAuthorFaves = -1;
    int favCount = -1;
    int reviewCount = -1;
    int wordCount = -1;
    int chapterCount = 0;
    int authorId = -1;

    QList<int> fandoms;
    QStringList genres;

    QString genreString;

    QDate published;
    QDate updated;


    friend  QTextStream &operator<<(QTextStream &out, const FanficDataForRecommendationCreation &p);
    friend  QTextStream &operator>>(QTextStream &in, FanficDataForRecommendationCreation &p);

    void Log();
    void Serialize(QDataStream &out);
    void Deserialize(QDataStream &in);

};
inline QTextStream &operator>>(QTextStream &in, FanficDataForRecommendationCreation &p);
inline QTextStream &operator<<(QTextStream &out, const FanficDataForRecommendationCreation &p);

typedef QSharedPointer<FanficDataForRecommendationCreation> FicWeightPtr;

struct FanficCompletionStatus{
    int ficId = -1;
    bool finished = false;
    int atChapter = -1;
};

struct FanficSnoozeStatus{
    int ficId = -1;
    bool untilFinished = false;
    int snoozedAtChapter = -1;
    int snoozedTillChapter = -1;
    bool expired = false;
    QDateTime added;
};
}
