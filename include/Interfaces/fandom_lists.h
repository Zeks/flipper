/*
Flipper is a replacement search engine for fanfiction.net search results
Copyright (C) 2017-2019  Marchenko Nikolai

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
#pragma once
#include "core/fandom_list.h"
#include <QSqlDatabase>
#include <functional>
#include <unordered_set>
#include <unordered_map>

namespace  {

}

namespace interfaces {

struct FandomListsImpl;

class FandomLists{
    using FandomList = core::fandom_lists::List;
    using FandomState = core::fandom_lists::FandomStateInList;
public:
    FandomLists();
    void Clear();
    void LoadFandomLists();
    // operations on fandoms
    void AddFandomToList(uint32_t listId,uint32_t fandomId);
    void RemoveFandomFromList(uint32_t listId,uint32_t fandomId);
    void EditFandomStateForList(const FandomState &);
    // operations on fandom lists themselves
    void AddFandomList(FandomList::ListPtr);
    void RemoveFandomList(uint32_t listId); // deactivate instead? or fuck precautions?
    void EditListState(FandomList::ListPtr);

    // where to put the code that creates the TreeInterface?
    // should I have only the raw representation here?

    // should I transition from using qt's database type?
    // nah, not the time to do it right now.
    // will prolly refactor all together
    // public until refactor to maintain uniformity of use between interface
    QSqlDatabase db;
private:
    std::unique_ptr<FandomListsImpl> d;
};

}
