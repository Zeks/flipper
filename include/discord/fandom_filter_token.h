#pragma once
#include <QList>
#include <QSet>
namespace discord {

struct FandomFilterToken{
    FandomFilterToken(){};
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
