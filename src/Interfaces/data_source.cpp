#include "Interfaces/data_source.h"
#include "pure_sql.h"

#include <QDebug>
#include <QSqlError>

FicFilter::FicFilter(){}
FicFilter::~FicFilter(){}
FicSource::FicSource(){}
FicSource::~FicSource(){}
FicSourceDirect::FicSourceDirect(QSharedPointer<database::IDBWrapper> dbInterface){
    this->db = dbInterface;
    std::unique_ptr<core::DefaultRNGgenerator> rng (new core::DefaultRNGgenerator());
    rng->portableDBInterface = dbInterface;
    queryBuilder.portableDBInterface = dbInterface;
    countQueryBuilder.portableDBInterface = dbInterface;
    queryBuilder.SetIdRNGgenerator(rng.release());
}
FicSourceDirect::~FicSourceDirect(){}

void FicSource::AddFicFilter(QSharedPointer<FicFilter> filter)
{
    filters.push_back(filter);
}

void FicSource::ClearFilters()
{
    filters.clear();
}

QSqlQuery FicSourceDirect::BuildQuery(core::StoryFilter filter, bool countOnly)
{
    QSqlDatabase db = QSqlDatabase::database();
    if(countOnly)
        currentQuery = countQueryBuilder.Build(filter);
    else
        currentQuery = queryBuilder.Build(filter);
    QSqlQuery q(db);
    q.prepare(currentQuery->str);
    auto it = currentQuery->bindings.begin();
    auto end = currentQuery->bindings.end();
    while(it != end)
    {
        if(currentQuery->str.contains(it.key()))
        {
            qDebug() << it.key() << " " << it.value();
            q.bindValue(it.key(), it.value());
        }
        ++it;
    }
    return q;
}

inline core::Fic FicSourceDirect::LoadFanfic(QSqlQuery& q)
{
    core::Fic result;
    result.id = q.value("ID").toInt();
    result.fandom = q.value("FANDOM").toString();
    result.author = core::Author::NewAuthor();
    result.author->name = q.value("AUTHOR").toString();
    result.title = q.value("TITLE").toString();
    result.summary = q.value("SUMMARY").toString();
    result.genreString = q.value("GENRES").toString();
    result.charactersFull = q.value("CHARACTERS").toString().replace("not found", "");
    result.rated = q.value("RATED").toString();
    auto published = q.value("PUBLISHED").toDateTime();
    auto updated   = q.value("UPDATED").toDateTime();
    result.published = published;
    result.updated= updated;
    result.SetUrl("ffn",q.value("URL").toString());
    result.tags = q.value("TAGS").toString();
    result.wordCount = q.value("WORDCOUNT").toString();
    result.favourites = q.value("FAVOURITES").toString();
    result.reviews = q.value("REVIEWS").toString();
    result.chapters = QString::number(q.value("CHAPTERS").toInt() + 1);
    result.complete= q.value("COMPLETE").toInt();
    result.atChapter = q.value("AT_CHAPTER").toInt();
    result.recommendations= q.value("SUMRECS").toInt();
    return result;
}

int FicSourceDirect::GetFicCount(core::StoryFilter filter)
{
    auto q = BuildQuery(filter, true);
    q.setForwardOnly(true);
    if(!database::puresql::ExecAndCheck(q))
        return -1;
    q.next();
    auto result =  q.value("records").toInt();
    return result;
}

void FicSourceDirect::FetchData(core::StoryFilter searchfilter, QList<core::Fic> * data)
{
    if(!data)
        return;

    auto q = BuildQuery(searchfilter);
    q.setForwardOnly(true);
    q.exec();
    qDebug().noquote() << q.lastQuery();
    if(q.lastError().isValid())
    {
        qDebug() << "Error loading data:" << q.lastError();
        qDebug().noquote() << q.lastQuery();
    }
    int counter = 0;
    data->clear();
    lastFicId = -1;
    while(q.next())
    {
        counter++;
        auto fic = LoadFanfic(q);
        bool filterOk = true;
        for(auto filter: filters)
            filterOk = filterOk && filter->Passed(&fic, searchfilter.slashFilter);
        if(filterOk)
            data->push_back(fic);
        if(counter%10000 == 0)
            qDebug() << "tick " << counter/1000;
    }
    if(data->size() > 0)
        lastFicId = data->last().id;
    qDebug() << "loaded fics:" << counter;
}

FicFilterSlash::FicFilterSlash()
{
    regexToken.Init();
}

FicFilterSlash::~FicFilterSlash(){}

bool FicFilterSlash::Passed(core::Fic * fic, const SlashFilterState& slashFilter)
{
    bool allow = true;
    SlashPresence slashToken;
    if(slashFilter.excludeSlash || slashFilter.includeSlash)
        slashToken = regexToken.ContainsSlash(fic->summary, fic->charactersFull, fic->fandom);

    if(slashFilter.applyLocalEnabled && slashFilter.excludeSlashLocal)
    {
        if(slashToken.IsSlash())
            allow = false;
    }
    if(slashFilter.applyLocalEnabled && slashFilter.includeSlashLocal)
    {
        if(!slashToken.IsSlash())
            allow = false;
    }
    return allow;
}
