#include "Interfaces/fandom_lists.h"
#include "pure_sql.h"


namespace interfaces {
struct FandomListsImpl{
    void Clear();
    using FandomList = core::fandom_lists::List;
    using FandomState = core::fandom_lists::FandomStateInList;
    // ?? use pointer for simplicity of editing or value to have a copy?
    // let's go with values for now
    // should I use pointers for ease of referencing? is there a point to keep these as values?
    std::unordered_map<uint16_t, FandomList::ListPtr> idToList;
    QMap<QString, FandomList::ListPtr> nameToList;
    // do I need a hash of all lists fandom is present in? can't hurt I guess
    std::unordered_map<int, std::unordered_set<int>> fandomToLists;
    std::unordered_map<int, std::vector<FandomState>> fandomStatesPerList; // storing values here prevents pre-declarations, d?
    std::vector<FandomState> dummy;
};

FandomLists::FandomLists():d(new FandomListsImpl)
{

}

void FandomLists::ProcessIgnoreListIntoFandomList()
{
    sql::ProcessIgnoresIntoFandomLists(db);
}

void FandomLists::Clear()
{
    d->Clear();
}

void FandomLists::LoadFandomLists()
{
    auto fandomLists = sql::FetchFandomLists(db);

    for(auto list : fandomLists.data){
        d->idToList[list->id] =  list;
        d->nameToList[list->name] =  list;
        auto fandomStates = sql::FetchFandomStatesInUserList(list->id, db);
        d->fandomStatesPerList[list->id].reserve(fandomStates.data.size());
        for(auto&& fandomState: fandomStates.data){
            d->fandomToLists[fandomState.id].insert(list->id);
            d->fandomStatesPerList[list->id].emplace_back(fandomState);
        }
    }
}

void FandomLists::AddFandomToList(uint32_t listId, uint32_t fandomId, QString fandomName)
{
    sql::AddFandomToUserList(listId, fandomId, fandomName, db);
}

void FandomLists::RemoveFandomFromList(uint32_t listId, uint32_t fandomId)
{
    sql::RemoveFandomFromUserList(listId, fandomId, db);
}

void FandomLists::EditFandomStateForList(const FandomLists::FandomState& fandomState)
{
    sql::EditFandomStateForList(fandomState, db);
}

int FandomLists::AddFandomList(QString name)
{
    return sql::AddNewFandomList(name, db).data;
}

void FandomLists::RemoveFandomList(uint32_t listId)
{
    sql::RemoveFandomList(listId, db);
}

void FandomLists::EditListState(const core::fandom_lists::List& list)
{
    sql::EditListState(list, db);
}

QStringList FandomLists::GetLoadedFandomLists() const
{
    return d->nameToList.keys();
}

core::fandom_lists::List::ListPtr FandomLists::GetFandomList(QString key) const
{
    auto it = d->nameToList.find(key);
    if(it != d->nameToList.end())
        return it.value();
    return nullptr;
}

const std::vector<FandomLists::FandomState> &FandomLists::GetFandomStatesForList(QString key) const
{
    auto it = d->nameToList.find(key);
    if(it == d->nameToList.end())
        return d->dummy;
    auto list = it.value();
    auto itStates = d->fandomStatesPerList.find(list->id);
    if(itStates == d->fandomStatesPerList.end())
        return d->dummy;
    return itStates->second;
}

void FandomLists::FlipValuesForList(uint32_t id)
{
    sql::FlipListValues(id, db);
}

void FandomListsImpl::Clear()
{
    idToList.clear();
    nameToList.clear();
    fandomToLists.clear();
    fandomStatesPerList.clear();
}




}
