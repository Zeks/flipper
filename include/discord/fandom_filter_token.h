/*Flipper is a recommendation and search engine for fanfiction.net
Copyright (C) 2017-2020  Marchenko Nikolai

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>*/
#pragma once
#include <QList>
#include <QSet>
namespace discord {

struct FandomFilterToken{
    FandomFilterToken()=default;
    FandomFilterToken(int id, bool includeCrossovers){
        this->id = id;
        this->includeCrossovers = includeCrossovers;
    }
    int id = -1;
    bool includeCrossovers = false;
};

struct FandomFilter{
    void RemoveFandom(int id){
        fandoms.remove(id);

        QList<FandomFilterToken> newTokens;
        for(auto token: qAsConst(tokens))
        {
            if(token.id != id)
                newTokens.push_back(token);
        }
        tokens =newTokens;
    }
    void AddFandom(int id, bool inludeCrossovers){
        fandoms.insert(id);
        tokens.push_back({id, inludeCrossovers});
    }
    FandomFilterToken GetToken(int id){
        for(auto& token : tokens)
            if(token.id == id)
                return token;
        return nullToken;
    }

    QSet<int> fandoms;
    QList<FandomFilterToken> tokens;
    FandomFilterToken nullToken;
};
}
Q_DECLARE_TYPEINFO(discord::FandomFilterToken, Q_MOVABLE_TYPE);
