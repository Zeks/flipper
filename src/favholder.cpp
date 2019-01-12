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
#include "include/favholder.h"
#include "include/Interfaces/authors.h"
#include "logger/QsLog.h"
#include "include/timeutils.h"
#include "threaded_data/threaded_save.h"
#include "threaded_data/threaded_load.h"
#include <QSettings>
#include <QDir>
#include <algorithm>
//#include <execution>
#include <cmath>
namespace core{

auto lambda = [](DataHolder* holder, QString storageFolder, QString fileBase, auto& data, auto interface, auto loadFunc, auto saveFunc)->void{
    holder->CreateTempDataDir(storageFolder);
    QSettings settings(holder->settingsFile, QSettings::IniFormat);
    if(settings.value("Settings/usestoreddata", true).toBool() && QFile::exists(storageFolder + "/" + fileBase + "_0.txt"))
        thread_boost::LoadData(storageFolder, fileBase, data);
    else
    {
        auto& item = data;
        item = loadFunc(interface);
        saveFunc(storageFolder);
    }
};

#define DISPATCH(X) \
    template <> \
    void DataHolder::LoadData<X>(QString storageFolder){ \
    auto[data, interface] = get<X>(); \
    lambda(this,storageFolder, QString::fromStdString(DataHolderInfo<X>::fileBase()), data.get(),interface, DataHolderInfo<X>::loadFunc(),\
    std::bind(&DataHolder::SaveData<X>, this, std::placeholders::_1));\
}

DISPATCH(rdt_favourites)
DISPATCH(rdt_fics)
//            template <>
//void DataHolder::LoadData<rdt_fics>(QString storageFolder){
//auto[data, interface] = get<rdt_fics>();
//    lambda(this, storageFolder,"", data.get(),interface, DataHolderInfo<rdt_fics>::loadFunc(),
//    std::bind(&DataHolder::SaveData<rdt_favourites>, this, std::placeholders::_1));
//}



void RecCalculator::LoadFavourites(QSharedPointer<interfaces::Authors> authorInterface)
{
    CreateTempDataDir();
    QSettings settings("settings_server.ini", QSettings::IniFormat);
    if(settings.value("Settings/usestoreddata", false).toBool() && QFile::exists("ServerData/roafav_0.txt"))
    {
        holder.LoadData<core::rdt_favourites>("ServerData");
        //LoadStoredFavouritesData();
    }
    else
    {
        LoadFavouritesDataFromDatabase(authorInterface);
        SaveFavouritesData();
    }
}

void RecCalculator::CreateTempDataDir()
{
    QDir dir(QDir::currentPath());
    dir.mkdir("ServerData");
}

void RecCalculator::LoadFavouritesDataFromDatabase(QSharedPointer<interfaces::Authors> authorInterface)
{
       Q_UNUSED(authorInterface)
    //    auto favourites = authorInterface->LoadFullFavouritesHashset();
    //    for(auto key: favourites.keys())
    //    {
    //        for(auto item : favourites[key])
    //        this->favourites[key].add(item);
    //    }
}

void RecCalculator::LoadStoredFavouritesData()
{
    //thread_boost::LoadFavouritesData("ServerData", favourites);
}

void RecCalculator::SaveFavouritesData()
{
    //thread_boost::SaveFavouritesData("ServerData", favourites);
}
enum class ECalcType{
    common,
    uncommon,
    near,
    close
};

double quadratic_coef(double ratio,
                      double median,
                      double sigma,
                      int authorSize,
                      int maximumMatches,
                      ECalcType type)
{
    Q_UNUSED(ratio);
    Q_UNUSED(median);
    Q_UNUSED(sigma);
    static QSet<int> bases;
    if(!bases.contains(authorSize))
    {
        bases.insert(authorSize);
        qDebug() << "Author size: " << authorSize;
    }
//    double base;
//    double distanceToNextLevel = 0;
//    double distanceToNextBase = 0;
//    double start = 0;
    double result = 0;
    switch(type)
    {
        case ECalcType::close:
        //qDebug() << "casting max vote: " << "matches: " << maximumMatches << " value: " << 0.2*static_cast<double>(maximumMatches);
        result =  0.2*static_cast<double>(maximumMatches);
        //base = ;
//        distanceToNextLevel = 9999;
//        if(base < 15)
//            base = 5;
        break;
    case ECalcType::near:
        //return 60;
    result = 0.05*static_cast<double>(maximumMatches);
//    distanceToNextLevel = 0.3*sigma;
//    distanceToNextBase = 0.2;
//    start = sigma*1.7;
//    if(base < 5)
//        base = 5;
    break;
    case ECalcType::uncommon:
    {
        //        return 5;
        auto val = 0.005*static_cast<double>(maximumMatches);
        result = val < 1 ? 1 : val;
    }
//    start = sigma;
//    distanceToNextLevel = 0.7*sigma;
//    distanceToNextBase = 0.07;
//    if(base < 2)
//        base = 2;
    break;
    default:
        result = 1;
    }
    return result;

//    auto traveled = (median - start - ratio)/(distanceToNextLevel);



//    auto result = base + traveled*distanceToNextBase*authorSize;;

//    auto castBase = static_cast<int>(base);
//    if(!bases.contains(castBase))
//    {
//        bases.insert(castBase);

//        qDebug() << "traveled = (median - start - ratio)/distanceToNextLevel";
//        qDebug() << traveled << " = ( " << median << " - " << start << " - " << ratio << " )/ " << distanceToNextLevel;

//        qDebug() << "Calculating: " << "author size: " << authorSize
//                 << " ratio: " << ratio << " median: " << median
//                     << " sigma: " << sigma
//                       << " base: " << base;
//        qDebug() << "vote result: " << result;
//    }
//    return result;
}

double sqrt_coef(double ratio, double median, double sigma, int base, int scaler)
{
    auto  distanceFromMedian = median - ratio;
    if(distanceFromMedian < 0)
        return 0;
    double tau = distanceFromMedian/sigma;
    return base + std::sqrt(tau)*scaler;
}




class RecCalculatorImplBase
{
public:
    typedef QList<std::function<bool(AuthorResult&,QSharedPointer<RecommendationList>)>> FilterListType;
    typedef QList<std::function<void(RecCalculatorImplBase*,AuthorResult &)>> ActionListType;

    RecCalculatorImplBase(const DataHolder::FavType& faves,
                          const DataHolder::FicType& fics):favs(faves),fics(fics){}

    virtual ~RecCalculatorImplBase(){}

    void Calc();
    void FetchAuthorRelations();
    void CollectFicMatchQuality();
    void Filter(QList<std::function<bool(AuthorResult&,QSharedPointer<RecommendationList>)>> filters,
                QList<std::function<void(RecCalculatorImplBase*,AuthorResult &)>> actions);
    virtual void CollectVotes();

    virtual void CalcWeightingParams() = 0;
    virtual FilterListType GetFilterList() = 0;
    virtual ActionListType  GetActionList() = 0;
    virtual std::function<AuthorWeightingResult(AuthorResult&, int, int)> GetWeightingFunc() = 0;


    int matchSum = 0;
    const DataHolder::FavType& favs;
    const DataHolder::FicType& fics;
    QSharedPointer<RecommendationList> params;
    //QList<int> matchedAuthors;
    QHash<uint32_t, core::FicWeightPtr> fetchedFics;
    QHash<int, AuthorResult> allAuthors;
    int maximumMatches = 0;
    int prevMaximumMatches = 0;
    QList<int> filteredAuthors;
    Roaring ownFavourites;
    RecommendationListResult result;
};

static auto matchesFilter = [](AuthorResult& author, QSharedPointer<RecommendationList> params){
    return author.matches >= params->minimumMatch || author.matches >= params->alwaysPickAt;
};
static auto ratioFilter = [](AuthorResult& author, QSharedPointer<RecommendationList> params)
{return author.ratio <= params->pickRatio && author.matches > 0;};
static auto authorAccumulator = [](RecCalculatorImplBase* ptr,AuthorResult & author)
{
    ptr->filteredAuthors.push_back(author.id);
};
//auto ratioAccumulator = [&ratioSum](RecCalculatorImplBase* ,AuthorResult & author){ratioSum+=author.ratio;};
class RecCalculatorImplDefault: public RecCalculatorImplBase{
public:
    RecCalculatorImplDefault(const DataHolder::FavType& faves, const DataHolder::FicType& fics): RecCalculatorImplBase(faves, fics){}
    virtual FilterListType GetFilterList(){
        return {matchesFilter, ratioFilter};
    }
    virtual ActionListType GetActionList(){
        return {authorAccumulator};
    }
    virtual std::function<AuthorWeightingResult(AuthorResult&, int, int)> GetWeightingFunc(){
        return [](AuthorResult&, int, int){return AuthorWeightingResult();};
    }
    void CalcWeightingParams(){
        // does nothing
    }
};
class RecCalculatorImplWeighted : public RecCalculatorImplBase{
public:
    RecCalculatorImplWeighted(const DataHolder::FavType& faves,const DataHolder::FicType& fics): RecCalculatorImplBase(faves, fics){}
    double ratioSum = 0;
    double ratioMedian = 0;
    double quad = 0;
    int sigma2Dist = 0;
    int counter2Sigma = 0;
    int counter17Sigma = 0;
    virtual FilterListType GetFilterList(){
        return {matchesFilter, ratioFilter};
    }
    virtual ActionListType GetActionList(){
        auto ratioAccumulator = [ratioSum = std::reference_wrapper<double>(this->ratioSum)](RecCalculatorImplBase* ,AuthorResult & author)
        {ratioSum+=author.ratio;};
        return {authorAccumulator, ratioAccumulator};
    };
    virtual std::function<AuthorWeightingResult(AuthorResult&, int, int)> GetWeightingFunc(){
        return std::bind(&RecCalculatorImplWeighted::CalcWeightingForAuthor,
                         this,
                         std::placeholders::_1,
                         std::placeholders::_2,
                         std::placeholders::_3);
    }

    void CalcWeightingParams(){
        int matchMedian = matchSum/favs.size();
        ratioMedian = static_cast<double>(ratioSum)/static_cast<double>(filteredAuthors.size());

        double normalizer = 1./static_cast<double>(filteredAuthors.size()-1.);
        double sum = 0;
        for(auto author: filteredAuthors)
        {
            sum+=std::pow(allAuthors[author].ratio - ratioMedian, 2);
        }
        quad = std::sqrt(normalizer * sum);


        qDebug () << "median value is: " << matchMedian;
        qDebug () << "median ratio is: " << ratioMedian;

        auto keysRatio = favs.keys();
        auto keysMedian = favs.keys();
        std::sort(keysMedian.begin(), keysMedian.end(),[&](const int& i1, const int& i2){
            return allAuthors[i1].matches < allAuthors[i2].matches;
        });

        std::sort(filteredAuthors.begin(), filteredAuthors.end(),[&](const int& i1, const int& i2){
            return allAuthors[i1].ratio < allAuthors[i2].ratio;
        });

        auto ratioMedianIt = std::lower_bound(filteredAuthors.begin(), filteredAuthors.end(), ratioMedian, [&](const int& i1, const int& ){
            return allAuthors[i1].ratio < ratioMedian;
        });
        auto dist = ratioMedianIt - filteredAuthors.begin();
        qDebug() << "distance to median is: " << dist;
        qDebug() << "vector size is: " << filteredAuthors.size();

        qDebug() << "sigma: " << quad;
        qDebug() << "2 sigma: " << quad * 2;

        auto ratioSigma2 = std::lower_bound(filteredAuthors.begin(), filteredAuthors.end(), ratioMedian, [&](const int& i1, const int& ){
            return allAuthors[i1].ratio < (ratioMedian - quad*2);
        });
        sigma2Dist = ratioSigma2 - filteredAuthors.begin();
        qDebug() << "distance to sigma15 is: " << sigma2Dist;
    }

    AuthorWeightingResult CalcWeightingForAuthor(AuthorResult& author, int authorSize, int maximumMatches){
        AuthorWeightingResult result;
        result.isValid = true;
        bool gtSigma = (ratioMedian - quad) >= author.ratio;
        bool gtSigma2 = (ratioMedian - 2 * quad) >= author.ratio;
        bool gtSigma17 = (ratioMedian - 1.7 * quad) >= author.ratio;

        if(gtSigma2)
        {

            result.authorType = AuthorWeightingResult::EAuthorType::unique;
            counter2Sigma++;
            //result.value = quadratic_coef(author.ratio,ratioMedian, quad, authorSize/10, authorSize/20, authorSize);
            result.value = quadratic_coef(author.ratio,ratioMedian, quad, authorSize, maximumMatches, ECalcType::close);
        }
        else if(gtSigma17)
        {
            result.authorType = AuthorWeightingResult::EAuthorType::rare;
            counter17Sigma++;
            //result.value  = quadratic_coef(author.ratio,ratioMedian,  quad,authorSize/20, authorSize/40, authorSize);
            result.value  = quadratic_coef(author.ratio,ratioMedian,  quad,  authorSize, maximumMatches, ECalcType::near);
        }
        else if(gtSigma)
        {
            result.authorType = AuthorWeightingResult::EAuthorType::uncommon;
            result.value  = quadratic_coef(author.ratio, ratioMedian, quad,  authorSize, maximumMatches, ECalcType::uncommon);
        }
        else
        {
            result.authorType = AuthorWeightingResult::EAuthorType::common;
        }
        return result;
    }
};
void RecCalculatorImplBase::Calc(){
    auto filters = GetFilterList();
    auto actions = GetActionList();

    TimedAction relations("Fetching relations",[&](){
        FetchAuthorRelations();
    });
    relations.run();
    TimedAction filtering("Filtering data",[&](){
        Filter(filters, actions);
    });
    filtering.run();
    TimedAction weighting("weighting",[&](){
        CalcWeightingParams();
    });
    weighting.run();

    TimedAction authorMatchQuality("Fetching match qualities for authors",[&](){
        CollectFicMatchQuality();
    });
    authorMatchQuality.run();


    TimedAction collecting("collecting votes ",[&](){
        CollectVotes();
    });
    collecting.run();
    TimedAction report("writing match report",[&](){
        for(auto& author: filteredAuthors)
            result.matchReport[allAuthors[author].matches]++;
    });
    report.run();
}
void RecCalculatorImplBase::CollectVotes()
{
    auto weightingFunc = GetWeightingFunc();
    auto authorSize = filteredAuthors.size();
    qDebug() << "Max Matches:" <<  prevMaximumMatches;
    std::for_each(filteredAuthors.begin(), filteredAuthors.end(), [this](int author){
        for(auto fic: favs[author])
        {
            result.recommendations[fic]+= 1;
        }
    });
    int maxValue = 0;
    int maxId = -1;

    auto it = result.recommendations.begin();
    while(it != result.recommendations.end())
    {
        if(it.value() > maxValue )
        {
            maxValue = it.value();
            maxId = it.key();
        }
        it++;
    }
//    for(auto fic: result.recommendations.keys()){
//        if(result.recommendations[fic] > maxValue )
//        {
//            maxValue = result.recommendations[fic];
//            maxId = fic;
//        }
//    }
    qDebug() << "Max pure votes: " << maxValue;
    qDebug() << "Max id: " << maxId;
    result.recommendations.clear();

    std::for_each(filteredAuthors.begin(), filteredAuthors.end(), [maxValue,weightingFunc, authorSize, this](int author){
        for(auto fic: favs[author])
        {
            auto weighting = weightingFunc(allAuthors[author],authorSize, maxValue );
            result.recommendations[fic]+= weighting.GetCoefficient();
            result.AddToBreakdown(fic, weighting.authorType, weighting.GetCoefficient());
        }
    });
}
void RecCalculatorImplBase::FetchAuthorRelations()
{
    qDebug() << "faves is of size: " << favs.size();
    allAuthors.reserve(favs.size());
    Roaring ignores;
    //QLOG_INFO() << "fandom ignore list is of size: " << params->ignoredFandoms.size();
    TimedAction ignoresCreation("Building ignores",[&](){
        for(auto fic: fics)
        {
            int count = 0;
            bool inIgnored = false;
            for(auto fandom: fic->fandoms)
            {
                if(fandom != -1)
                    count++;
                if(params->ignoredFandoms.contains(fandom) && fandom > 1)
                    inIgnored = true;
            }
            if(/*count == 1 && */inIgnored)
                ignores.add(fic->id);

        }
    });
    ignoresCreation.run();
    QLOG_INFO() << "fanfic ignore list is of size: " << ignores.cardinality();


    auto sourceFics = QSet<uint32_t>::fromList(fetchedFics.keys());

    for(auto bit : sourceFics)
        ownFavourites.add(bit);
    qDebug() << "finished creating roaring";
    int minMatches;
    minMatches =  params->minimumMatch;
    maximumMatches = minMatches;
    TimedAction action("Relations Creation",[&](){
        auto it = favs.begin();
        while (it != favs.end())
        {
            if(params->userFFNId == it.key())
            {
                QLOG_INFO() << "Skipping user's own list: " << params->userFFNId ;
                it++;
                continue;
            }

            auto& author = allAuthors[it.key()];
            author.id = it.key();
            author.matches = 0;
            //QLOG_INFO
            //these are the fics from the current fav list that are ignored

            //bool hasIgnoredMatches = false;
            Roaring ignoredTemp = it.value();
            ignoredTemp = ignoredTemp & ignores;

            if(ignoredTemp.cardinality() > 0)
            {
                //hasIgnoredMatches = true;
//                QLOG_INFO() << "ficl list size is: " << it.value().cardinality();
//                QLOG_INFO() << "of those ignored are: " << ignoredTemp.cardinality();
            }
            author.size = it.value().cardinality();
            Roaring temp = ownFavourites;
            // first we need to remove ignored fics
            auto unignoredSize = it.value().xor_cardinality(ignoredTemp);
//            if(hasIgnoredMatches)
//                QLOG_INFO() << "this leaves unignored: " << unignoredSize;
            temp = temp & it.value();
            author.matches = temp.cardinality();
            author.sizeAfterIgnore = unignoredSize;
            if(ignores.cardinality() == 0)
                author.sizeAfterIgnore = author.size;
            if(maximumMatches < author.matches)
            {
                prevMaximumMatches = maximumMatches;
                maximumMatches = author.matches;
            }
            matchSum+=author.matches;
            it++;
        }
    });
    action.run();
}

void RecCalculatorImplBase::CollectFicMatchQuality()
{
    QVector<int> tempAuthors;

    for(auto authorId : filteredAuthors)
    {
        tempAuthors.push_back(authorId);
        auto& author = allAuthors[authorId];
        auto& authorFaves = favs[authorId];
        Roaring temp = ownFavourites;
        auto matches = temp & authorFaves;
        for(auto value : matches)
        {
            auto itFic = fics.find(value);
            if(itFic == fics.end())
                continue;

            // < 500 1 votes
            // < 300 4 votes
            // < 100 8 votes
            // < 50 15 votes

            author.breakdown.total++;
            if(itFic->get()->favCount <= 50)
            {
                author.breakdown.below50++;
                author.breakdown.votes+=15;
                if(itFic->get()->authorId != -1)
                    author.breakdown.authors.insert(itFic->get()->authorId);

            }
            else if(itFic->get()->favCount <= 100)
            {
                author.breakdown.below100++;
                author.breakdown.votes+=8;
                if(itFic->get()->authorId != -1)
                    author.breakdown.authors.insert(itFic->get()->authorId);
            }
            else if(itFic->get()->favCount <= 300)
            {
                author.breakdown.below300++;
                author.breakdown.votes+=4;
                if(itFic->get()->authorId != -1)
                    author.breakdown.authors.insert(itFic->get()->authorId);
            }
            else if(itFic->get()->favCount <= 500)
            {
                author.breakdown.below500++;
                author.breakdown.votes+=1;
            }
        }
        author.breakdown.priority1 = (author.breakdown.below50 + author.breakdown.below100) >= 3;
        author.breakdown.priority1 = author.breakdown.priority1 && author.breakdown.authors.size() > 1;

        author.breakdown.priority2 = (author.breakdown.below50 + author.breakdown.below100 + author.breakdown.below300) >= 5;
        author.breakdown.priority2 = author.breakdown.priority1 || (author.breakdown.priority2 && author.breakdown.authors.size() > 1);

        author.breakdown.priority3 = (author.breakdown.below50 + author.breakdown.below100 + author.breakdown.below300 + author.breakdown.below500) >= 8;
        author.breakdown.priority3 = (author.breakdown.priority1 || author.breakdown.priority2) || (author.breakdown.priority3 && author.breakdown.authors.size() > 1);
    }
    std::sort(tempAuthors.begin(), tempAuthors.end(), [&](int id1, int id2){

        bool largerScore = allAuthors[id1].breakdown.votes > allAuthors[id2].breakdown.votes;
        bool doubleBelow100First =  (allAuthors[id1].breakdown.below50 + allAuthors[id1].breakdown.below100) >= 2;
        bool doubleBelow100Second =  (allAuthors[id2].breakdown.below50 + allAuthors[id2].breakdown.below100) >= 2;
        if(doubleBelow100First && !doubleBelow100Second)
            return true;
        if(!doubleBelow100First && doubleBelow100Second)
            return false;
        return largerScore;
    });

    QLOG_INFO() << "Displaying 100 most closesly matched authors";
    auto justified = [](int value, int padding) {
        return QString::number(value).leftJustified(padding, ' ');
    };
    QLOG_INFO() << "Displaying priority lists: ";
    QLOG_INFO() << "//////////////////////";
    QLOG_INFO() << "P1: ";
    QLOG_INFO() << "//////////////////////";
    for(int i = 0; i < 100; i++)
    {
        if(allAuthors[tempAuthors[i]].breakdown.priority1)
            QLOG_INFO() << "Author id: " << justified(tempAuthors[i],9) << " votes: " <<  justified(allAuthors[tempAuthors[i]].breakdown.votes,5) <<
                       "Ratio: " << justified(allAuthors[tempAuthors[i]].ratio, 3) <<
                       "below 50: " << justified(allAuthors[tempAuthors[i]].breakdown.below50, 5) <<
                       "below 100: " << justified(allAuthors[tempAuthors[i]].breakdown.below100, 5) <<
                       "below 300: " << justified(allAuthors[tempAuthors[i]].breakdown.below300, 5) <<
                       "below 500: " << justified(allAuthors[tempAuthors[i]].breakdown.below500, 5);
    }
    QLOG_INFO() << "//////////////////////";
    QLOG_INFO() << "P2: ";
    QLOG_INFO() << "//////////////////////";
    for(int i = 0; i < 100; i++)
    {
        if(allAuthors[tempAuthors[i]].breakdown.priority2)
            QLOG_INFO() << "Author id: " << justified(tempAuthors[i],9) << " votes: " <<  justified(allAuthors[tempAuthors[i]].breakdown.votes,5) <<
                       "Ratio: " << justified(allAuthors[tempAuthors[i]].ratio, 3) <<
                       "below 50: " << justified(allAuthors[tempAuthors[i]].breakdown.below50, 5) <<
                       "below 100: " << justified(allAuthors[tempAuthors[i]].breakdown.below100, 5) <<
                       "below 300: " << justified(allAuthors[tempAuthors[i]].breakdown.below300, 5) <<
                       "below 500: " << justified(allAuthors[tempAuthors[i]].breakdown.below500, 5);
    }
    QLOG_INFO() << "//////////////////////";
    QLOG_INFO() << "P3: ";
    QLOG_INFO() << "//////////////////////";
    for(int i = 0; i < 100; i++)
    {
        if(allAuthors[tempAuthors[i]].breakdown.priority3)
            QLOG_INFO() << "Author id: " << justified(tempAuthors[i],9) << " votes: " <<  justified(allAuthors[tempAuthors[i]].breakdown.votes,5) <<
                        "Ratio: " << justified(allAuthors[tempAuthors[i]].ratio, 3) <<
                       "below 50: " << justified(allAuthors[tempAuthors[i]].breakdown.below50, 5) <<
                       "below 100: " << justified(allAuthors[tempAuthors[i]].breakdown.below100, 5) <<
                       "below 300: " << justified(allAuthors[tempAuthors[i]].breakdown.below300, 5) <<
                       "below 500: " << justified(allAuthors[tempAuthors[i]].breakdown.below500, 5);
    }

}

void RecCalculatorImplBase::Filter(QList<std::function<bool (AuthorResult &, QSharedPointer<RecommendationList>)> > filters,
                                   QList<std::function<void (RecCalculatorImplBase *, AuthorResult &)> > actions)
{
    auto params = this->params;
    auto thisPtr = this;
    std::for_each(allAuthors.begin(), allAuthors.end(), [filters, actions, params,thisPtr](AuthorResult& author){
        author.ratio = author.matches != 0 ? static_cast<double>(author.sizeAfterIgnore)/static_cast<double>(author.matches) : 999999;
        bool fail = std::any_of(filters.begin(), filters.end(), [&](decltype(filters)::value_type filter){
                return filter(author, params) == 0;
    });
        if(fail)
            return;
        std::for_each(actions.begin(), actions.end(), [thisPtr, &author](decltype(actions)::value_type action){
            action(thisPtr, author);
        });

    });
}




RecommendationListResult RecCalculator::GetMatchedFicsForFavList(QHash<uint32_t, core::FicWeightPtr> fetchedFics,
                                                                 QSharedPointer<RecommendationList> params)
{
    QSharedPointer<RecCalculatorImplBase> calculator;
    if(params->useWeighting)
        calculator.reset(new RecCalculatorImplWeighted(holder.faves, holder.fics));
    else
        calculator.reset(new RecCalculatorImplDefault(holder.faves, holder.fics));
    calculator->fetchedFics = fetchedFics;
    calculator->params = params;
    TimedAction action("Reclist Creation",[&](){
        calculator->Calc();
    });
    action.run();
    calculator->result.authors = calculator->filteredAuthors;

    return calculator->result;
}

// unused logging chunk
//QList<AuthorResult> authors = authorsResult.values();
//std::sort(authors.begin(), authors.end(), [](const AuthorResult& a1, const AuthorResult& a2){
//    return a1.matches > a2.matches;
//});
//qDebug() << "/////////////////// AUTHOR IDs: ////////////////";
//int counter = 0;
//for(int i = 0; counter < 15 && i < authors.size(); i++)
//{
//    if(authors[i].size < 800)
//    {
//        qDebug() << authors[i].id;
//        counter++;
//    }
//}
//return ficResult;

}
