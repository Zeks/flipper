#pragma once
#include <QRegExp>

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

NarrowResult GetNextInstanceOf(QString text, QString regex1, QString regex2, bool forward);
QString GetSingleNarrow(QString text,QString regex1, QString regex2, bool forward1);
QString GetDoubleNarrow(QString text,
                        QString regex1, QString regex2, bool forward1,
                        QString regex3, QString regex4, bool forward2,
                        int lengthOfLastTag);
