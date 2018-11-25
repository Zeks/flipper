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
        LoadStoredFavouritesData();
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
    static QSet<int> bases;
    if(!bases.contains(authorSize))
    {
        bases.insert(authorSize);
        qDebug() << "Author size: " << authorSize;
    }
    double base;
    double distanceToNextLevel = 0;
    double distanceToNextBase = 0;
    double start = 0;
    switch(type)
    {
        case ECalcType::close:
        return 0.2*static_cast<double>(maximumMatches);
        //base = ;
        distanceToNextLevel = 9999;
        if(base < 15)
            base = 5;
        break;
    case ECalcType::near:
        //return 60;
    return  0.05*static_cast<double>(maximumMatches);
    distanceToNextLevel = 0.3*sigma;
    distanceToNextBase = 0.2;
    start = sigma*1.7;
    if(base < 5)
        base = 5;
    break;
    case ECalcType::uncommon:
//        return 5;
    return  0.02*static_cast<double>(maximumMatches);
    start = sigma;
    distanceToNextLevel = 0.7*sigma;
    distanceToNextBase = 0.07;
    if(base < 2)
        base = 2;
    break;
    }

    auto traveled = (median - start - ratio)/(distanceToNextLevel);



    auto result = base + traveled*distanceToNextBase*authorSize;;

    auto castBase = static_cast<int>(base);
    if(!bases.contains(castBase))
    {
        bases.insert(castBase);

        qDebug() << "traveled = (median - start - ratio)/distanceToNextLevel";
        qDebug() << traveled << " = ( " << median << " - " << start << " - " << ratio << " )/ " << distanceToNextLevel;

        qDebug() << "Calculating: " << "author size: " << authorSize
                 << " ratio: " << ratio << " median: " << median
                     << " sigma: " << sigma
                       << " base: " << base;
        qDebug() << "vote result: " << result;
    }
    return result;
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

    RecCalculatorImplBase(const DataHolder::FavType& faves):favs(faves){}
    virtual ~RecCalculatorImplBase(){}

    void Calc();
    void FetchAuthorRelations();
    void Filter(QList<std::function<bool(AuthorResult&,QSharedPointer<RecommendationList>)>> filters,
                QList<std::function<void(RecCalculatorImplBase*,AuthorResult &)>> actions);
    virtual void CollectVotes();

    virtual void CalcWeightingParams() = 0;
    virtual FilterListType GetFilterList() = 0;
    virtual ActionListType  GetActionList() = 0;
    virtual std::function<AuthorWeightingResult(AuthorResult&, int, int)> GetWeightingFunc() = 0;


    int matchSum = 0;
    const DataHolder::FavType& favs;
    QSharedPointer<RecommendationList> params;
    QHash<uint32_t, core::FicWeightPtr> fetchedFics;
    QHash<int, AuthorResult> allAuthors;
    int maximumMatches = 0;
    int prevMaximumMatches = 0;
    QList<int> filteredAuthors;
    RecommendationListResult result;
};

static auto matchesFilter = [](AuthorResult& author, QSharedPointer<RecommendationList> params){
    return author.matches >= params->minimumMatch || author.matches >= params->alwaysPickAt;
};
static auto ratioFilter = [](AuthorResult& author, QSharedPointer<RecommendationList> params)
{return author.ratio <= params->pickRatio && author.matches > 0;};
static auto authorAccumulator = [](RecCalculatorImplBase* ptr,AuthorResult & author){ptr->filteredAuthors.push_back(author.id);};
//auto ratioAccumulator = [&ratioSum](RecCalculatorImplBase* ,AuthorResult & author){ratioSum+=author.ratio;};
class RecCalculatorImplDefault: public RecCalculatorImplBase{
public:
    RecCalculatorImplDefault(const DataHolder::FavType& faves): RecCalculatorImplBase(faves){}
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
    RecCalculatorImplWeighted(const DataHolder::FavType& faves): RecCalculatorImplBase(faves){}
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
        auto ratioAccumulator = [ratioSum = std::reference_wrapper<double>(this->ratioSum)](RecCalculatorImplBase* ptr,AuthorResult & author)
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

        auto ratioMedianIt = std::lower_bound(filteredAuthors.begin(), filteredAuthors.end(), ratioMedian, [&](const int& i1, const int& i2){
            return allAuthors[i1].ratio < ratioMedian;
        });
        auto dist = ratioMedianIt - filteredAuthors.begin();
        qDebug() << "distance to median is: " << dist;
        qDebug() << "vector size is: " << filteredAuthors.size();

        qDebug() << "sigma: " << quad;
        qDebug() << "2 sigma: " << quad * 2;

        auto ratioSigma2 = std::lower_bound(filteredAuthors.begin(), filteredAuthors.end(), ratioMedian, [&](const int& i1, const int& i2){
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
    FetchAuthorRelations();
    Filter(filters, actions);
    CalcWeightingParams();
    CollectVotes();

}
void RecCalculatorImplBase::CollectVotes()
{
    auto weightingFunc = GetWeightingFunc();
    auto authorSize = filteredAuthors.size();
    qDebug() << "Max Matches:" <<  prevMaximumMatches;
    std::for_each(filteredAuthors.begin(), filteredAuthors.end(), [weightingFunc, authorSize, this](int author){
        for(auto fic: favs[author])
        {
            result.recommendations[fic]+= 1;
        }
    });
    int maxValue = 0;
    int maxId = -1;

    for(auto fic: result.recommendations.keys()){
        if(result.recommendations[fic] > maxValue )
        {
            maxValue = result.recommendations[fic];
            maxId = fic;
        }
    }
    qDebug() << "Max pure votes: " << maxValue;
    qDebug() << "Max id: " << maxId;
    result.recommendations.clear();

    std::for_each(filteredAuthors.begin(), filteredAuthors.end(), [maxValue,weightingFunc, authorSize, this](int author){
        for(auto fic: favs[author])
        {
            auto weighting = weightingFunc(allAuthors[author],maxValue, authorSize);
            result.recommendations[fic]+= weighting.GetCoefficient();
            result.AddToBreakdown(fic, weighting.authorType, weighting.GetCoefficient());
        }
    });
}
void RecCalculatorImplBase::FetchAuthorRelations()
{
    qDebug() << "faves is of size: " << favs.size();
    auto sourceFics = QSet<uint32_t>::fromList(fetchedFics.keys());
    Roaring r;
    for(auto bit : sourceFics)
        r.add(bit);
    qDebug() << "finished creating roaring";
    int minMatches;
    minMatches =  params->minimumMatch;
    maximumMatches = minMatches;
    TimedAction action("Relations Creation",[&](){
        auto it = favs.begin();
        while (it != favs.end())
        {
            auto& author = allAuthors[it.key()];
            author.id = it.key();
            author.matches = 0;
            author.size = it.value().cardinality();
            Roaring temp = r;
            temp = temp & it.value();
            author.matches = temp.cardinality();
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

void RecCalculatorImplBase::Filter(QList<std::function<bool (AuthorResult &, QSharedPointer<RecommendationList>)> > filters,
                                   QList<std::function<void (RecCalculatorImplBase *, AuthorResult &)> > actions)
{
    auto params = this->params;
    auto thisPtr = this;
    std::for_each(allAuthors.begin(), allAuthors.end(), [filters, actions, params,thisPtr](AuthorResult& author){
        author.ratio = author.matches != 0 ? static_cast<double>(author.size)/static_cast<double>(author.matches) : 999999;
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
        calculator.reset(new RecCalculatorImplWeighted(holder.faves));
    else
        calculator.reset(new RecCalculatorImplDefault(holder.faves));
    calculator->fetchedFics = fetchedFics;
    calculator->params = params;
    TimedAction action("Reclist Creation",[&](){
        calculator->Calc();
    });
    action.run();

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
