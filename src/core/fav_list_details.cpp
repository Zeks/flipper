#include "core/fav_list_details.h"
#include <QDebug>
#include <QDataStream>

namespace core {

void core::FavListDetails::Serialize(QDataStream &out)
{
    out << favourites;
    out << ficWordCount;
    out << averageWordsPerChapter;
    out << averageLength;
    out << fandomsDiversity;
    out << explorerFactor;
    out << megaExplorerFactor;
    out << crossoverFactor;
    out << unfinishedFactor;
    out << esrbUniformityFactor;
    out << esrbKiddy;
    out << esrbMature;
    out << genreDiversityFactor;
    out << moodUniformity;
    out << moodNeutral;
    out << moodSad;
    out << moodHappy;
    out << crackRatio;
    out << slashRatio;
    out << notSlashRatio;
    out << smutRatio;
    out << static_cast<int>(esrbType);
    out << static_cast<int>(prevalentMood);
    out << static_cast<int>(mostFavouritedSize);
    out << static_cast<int>(sectionRelativeSize);

    out << prevalentGenre;
    out << sizeFactors;
    out << fandoms;
    out << fandomFactors;
    out << fandomsConverted;
    out << fandomFactorsConverted;
    out << genreFactors;

    out << firstPublished;
    out << lastPublished;

}

void core::FavListDetails::Deserialize(QDataStream &in)
{
    in >>  favourites;
    in >>  ficWordCount;
    in >>  averageWordsPerChapter;
    in >>  averageLength;
    in >>  fandomsDiversity;
    in >>  explorerFactor;
    in >>  megaExplorerFactor;
    in >>  crossoverFactor;
    in >>  unfinishedFactor;
    in >>  esrbUniformityFactor;
    in >>  esrbKiddy;
    in >>  esrbMature;
    in >>  genreDiversityFactor;
    in >>  moodUniformity;
    in >>  moodNeutral;
    in >>  moodSad;
    in >>  moodHappy;
    in >>  crackRatio;
    in >>  slashRatio;
    in >>  notSlashRatio;
    in >>  smutRatio;
    int temp;
    in >>  temp;
    esrbType = static_cast<ESRBType>(temp);
    in >>  temp;
    prevalentMood = static_cast<MoodType>(temp);
    in >>  temp;
    mostFavouritedSize = static_cast<EntitySizeType>(temp);
    in >>  temp;
    sectionRelativeSize = static_cast<EntitySizeType>(temp);

    in >>  prevalentGenre;
    in >>  sizeFactors;
    in >>  fandoms;
    in >>  fandomFactors;
    in >>  fandomsConverted;
    in >>  fandomFactorsConverted;
    in >>  genreFactors;

    in >>  firstPublished;
    in >>  lastPublished;
}


}
