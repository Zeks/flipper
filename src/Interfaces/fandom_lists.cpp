#include "Interfaces/fandom_lists.h"


namespace interfaces {
struct FandomListsImpl{
    void Clear();
    using FandomList = core::fandom_lists::List;
    using FandomState = core::fandom_lists::FandomStateInList;
    // ?? use pointer for simplicity of editing or value to have a copy?
    // let's go with values for now
    // should I use pointers for ease of referencing? is there a point to keep these as values?
    std::unordered_map<uint16_t, FandomList::ListPtr> idToList;
    std::unordered_map<std::string, FandomList::ListPtr> nameToList;
    // do I need a hash of all lists fandom is present in? can't hurt I guess
    std::unordered_map<int, std::unordered_set<int>> fandomToLists;
    std::unordered_map<int, std::vector<FandomState>> fandomStatesPerList; // storing values here prevents pre-declarations, d?
};

FandomLists::FandomLists():d(new FandomListsImpl)
{

}

void FandomLists::Clear()
{
    d->Clear();
}

void FandomLists::LoadFandomLists()
{
    auto fandomLists = database::puresql::FetchFandomLists(db);

    for(auto list : fandomLists.data){
        d->idToList[list->listId] =  list;
        d->nameToList[list->listName] =  list;
        auto fandomStates = database::puresql::FetchFandomStatesInUserList(list->listId, db);
        d->fandomStatesPerList[list->listId].reserve(fandomStates.data.size());
        for(auto&& fandomState: fandomStates.data){
            d->fandomToLists[fandomState.fandom_id].insert(list->listId);
            d->fandomStatesPerList[list->listId].emplace_back(fandomState);
        }
    }
}

void FandomLists::AddFandomToList(uint32_t listId, uint32_t fandomId)
{
    database::puresql::AddFandomToUserList(listId, fandomId, db);
}

void FandomLists::RemoveFandomFromList(uint32_t listId, uint32_t fandomId)
{
    database::puresql::RemoveFandomFromUserList(listId, fandomId, db);
}

void FandomLists::EditFandomStateForList(const FandomLists::FandomState& fandomState)
{
    database::puresql::EditFandomStateForList(fandomState, db);
}

void FandomLists::EditListState(core::fandom_lists::List::ListPtr list)
{
    database::puresql::EditListState(list, db);
}

void FandomListsImpl::Clear()
{
    idToList.clear();
    nameToList.clear();
    fandomToLists.clear();
    fandomStatesPerList.clear();
}




}
