#pragma once
#include <QRegExp>
#include <QList>
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

    //SearchToken(QList<RegularExpressionToken> rx, int move, bool first){
//        this->regex = rx;
//        QString reverseSearch;
//        this->reversedRegex = rx;

//        this->forwardDirection = first;
//        std::reverse(reversedRegex.begin(), reversedRegex.end());
//        this->reversedRegex.replace("s\\", "\\s");
//        this->reversedRegex.replace("d\\", "\\d");

    //}
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

QString BouncingSearch(QString str, QList<SearchToken> tokens);

NarrowResult GetNextInstanceOf(QString text, QString regex1, QString regex2, bool forward);
QString GetSingleNarrow(QString text,QString regex1, QString regex2, bool forward1);
QString GetDoubleNarrow(QString text,
                        QString regex1, QString regex2, bool forward1,
                        QString regex3, QString regex4, bool forward2,
                        int lengthOfLastTag);

