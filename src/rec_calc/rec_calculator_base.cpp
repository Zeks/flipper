#include "include/rec_calc/rec_calculator_base.h"
#include "timeutils.h"

namespace core{
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

    // not very satisfied with the results
//    TimedAction authorMatchQuality("Fetching match qualities for authors",[&](){
//        CollectFicMatchQuality();
//    });
//    authorMatchQuality.run();


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

Roaring RecCalculatorImplBase::BuildIgnoreList()
{
    Roaring ignores;
    //QLOG_INFO() << "fandom ignore list is of size: " << params->ignoredFandoms.size();
    QLOG_INFO() << "Building ignore list";
    TimedAction ignoresCreation("Building ignores",[&](){
        for(auto fic: fics)
        {
            if(!fic)
                continue;

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
    return ignores;
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



}
