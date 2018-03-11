#pragma once
#include <QRegExp>
#include <QRegularExpression>
#include <QList>
#include <QHash>
struct NarrowResult{
    NarrowResult(int first, int second){
        this->first = first;
        this->second = second;
        length = std::abs(std::abs(second) - std::abs(first));
    }
    int first = -1;
    int second = -1;
    int length = -1;
    int Length() const {return length;}
    bool IsValid() const {return first != -1 && second != -1;}
};

struct RegularExpressionToken{
    RegularExpressionToken();
    RegularExpressionToken(const char* data, int type){
        token = QString::fromLocal8Bit(data);
        tokenType = type;
        if(type == 0)
        {
            reverseToken = token;
            std::reverse(reverseToken.begin(), reverseToken.end());
        }
    }
    QString data(){
        return token;
    }
    QString rdata(){
        if(tokenType == 0)
            return reverseToken;
        else
            return token;
    }
    int tokenType = -1; // 0 - string, 1 - control character
    QString token;
    QString reverseToken;

};

RegularExpressionToken operator"" _s ( const char* data, size_t len);
RegularExpressionToken operator"" _c ( const char* data, size_t len);
namespace SearchTokenNamespace{
enum EBoundaryType
{
    move_left_boundary = 0,
    move_right_boundary = 1,
};
enum ELookupType
{
    find_first_instance= 0,
    find_last_instance = 1,
};
}


struct SearchToken
{
    SearchToken(QString rx, QString bitmask, int move, SearchTokenNamespace::ELookupType lookupType, SearchTokenNamespace::EBoundaryType boundaryType)
    {
        this->moveAmount = move;
        this->forwardDirection = lookupType == SearchTokenNamespace::find_first_instance;
        this->snapLeftBound = boundaryType == SearchTokenNamespace::move_left_boundary;
        bitmask = bitmask.replace(" ", "");
        if(bitmask.size() > 0)
        {
            QList<RegularExpressionToken> tokens;
            QString currentGroupText;
            QString currentBitmaskCharacter = bitmask.left(1);
            QString oldBitmaskCharacter = bitmask.left(1);

            for(int i = 0; i < rx.size(); i++)
            {
                QString character = rx.at(i);
                QString bitmaskCharacter = bitmask.at(i);
                if(!currentGroupText.isEmpty() && bitmaskCharacter != currentBitmaskCharacter)
                {
                    currentBitmaskCharacter = bitmaskCharacter;
                    tokens.push_back({qPrintable(currentGroupText), oldBitmaskCharacter == "1"});
                    oldBitmaskCharacter = currentBitmaskCharacter;
                    currentGroupText = "";
                }
                currentGroupText+=character;
            }
            if(!currentGroupText.isEmpty())
                tokens.push_back({qPrintable(currentGroupText), oldBitmaskCharacter == "1"});
            std::reverse(tokens.begin(), tokens.end());
            for(auto token: tokens)
            {
                this->reversedRegex+=token.rdata();
            }
            regex = rx;
        }
    }


    bool findFirst = true;
    bool forwardDirection = true;
    int moveAmount = 0;
    QString regex;
    QString reversedRegex;
    bool snapLeftBound = true;
};

struct FieldSearcher
{
    QString name;
    QList<SearchToken> tokens;
};


QString BouncingSearch(QString str, FieldSearcher tokens);

NarrowResult GetNextInstanceOf(QString text, QString regex1, QString regex2, bool forward);
QString GetSingleNarrow(QString text,QString regex1, QString regex2, bool forward1);
QString GetDoubleNarrow(QString text,
                        QString regex1, QString regex2, bool forward1,
                        QString regex3, QString regex4, bool forward2,
                        int lengthOfLastTag);
//namespace core{ class Fic;}
struct SlashPresence{
    bool containsSlash = false;
    bool containsNotSlash = false;
    bool IsSlash(){return containsSlash && !containsNotSlash;}
};
struct CommonRegex
{
    CommonRegex(){Init();}
    void Init();
    void Log();
    SlashPresence ContainsSlash(QString summary, QString characters, QString fandoms) const;
    bool initComplete = false;
    QHash<QString, QString> slashRegexPerFandom;
    QHash<QString, QString> characterSlashPerFandom;

    QString universalSlashRegex;
    QString notSlash; // universal not slash
    QString smut; // universal smut


    QHash<QString, QRegularExpression> rxHashSlashFandom;
    QHash<QString, QRegularExpression> rxHashCharacterSlashFandom;

    QRegularExpression rxUniversal;
    QRegularExpression rxNotSlash; // universal not slash
    QRegularExpression rxSmut; // universal smut


};

QString GetSlashRegex();

