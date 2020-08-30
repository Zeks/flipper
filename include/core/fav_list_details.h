#pragma once
#include <QHash>
#include <QMap>
#include <QDate>
namespace core {


enum class EntitySizeType{
    none = 0,
    small = 1,
    medium = 2,
    big = 3,
    huge = 4
};


class FicSectionStatsTemporaryToken
{
    public:
    QHash<int, int> ficSizeKeeper;
    QHash<int, int> crossKeeper;
    QHash<int, int> favouritesSizeKeeper;
    QHash<int, int> popularUnpopularKeeper;
    QHash<QString, int> fandomKeeper;
    QHash<int, int> unfinishedKeeper;
    QHash<int, int> esrbKeeper;
    QHash<int, int> wordsKeeper;
    QHash<QString, int> genreKeeper;
    QHash<int, int> moodKeeper;
    int chapterKeeper = 0;
    QList<int> sizes;
    QDate firstPublished;
    QDate lastPublished;
    int ficCount = 0;
    int wordCount = 0;
    int authorId = -1;
    QDate bioLastUpdated;
    QDate pageCreated;
};



class FavListDetails{
public:
    enum class FavouritesType{
        tiny = 0 ,
        medium = 1,
        large = 2,
        bs = 3
    };
    enum class MoodType{
        sad = 0,
        neutral = 1,
        positive = 2,
    };
    enum class ESRBType{
        agnostic = 0,
        kiddy = 1,
        mature = 2,
    };
    bool isValid = false;
    int favourites = -1;
    int noInfoCount = 0;
    int ficWordCount = 0;

    double averageWordsPerChapter = 0;
    double averageLength = 0.0;
    double fandomsDiversity = 0.0;
    double explorerFactor = 0.0;
    double megaExplorerFactor = 0.0;
    double crossoverFactor = 0.0;
    double unfinishedFactor = 0.0;
    double esrbUniformityFactor = 0.0;
    double esrbKiddy = 0.0;
    double esrbMature= 0.0;
    double genreDiversityFactor = 0.0;
    double moodUniformity = 0.0;
    double moodNeutral = 0.0;
    double moodSad = 0.0;
    double moodHappy = 0.0;


    double crackRatio = 0.0;
    double slashRatio = 0.0;
    double notSlashRatio = 0.0;
    double smutRatio = 0.0;

    ESRBType esrbType;
    MoodType prevalentMood = MoodType::neutral;
    EntitySizeType mostFavouritedSize;
    EntitySizeType sectionRelativeSize;

    QString prevalentGenre;
    QHash<QString, double> genreFactors;

    QMap<int, double> sizeFactors;

    QHash<QString, int> fandoms;
    QHash<QString, double> fandomFactors;

    QHash<int, int> fandomsConverted;
    QHash<int, double> fandomFactorsConverted;



    QDate firstPublished;
    QDate lastPublished;

    void Serialize(QDataStream &out);
    void Deserialize(QDataStream &in);
};
}
