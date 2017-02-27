#pragma once
#include <QString>
#include <QVariant>

struct ParseToken
{
    QString name;
    QString matchedValue;
    QString fixedValue;
    QString valueAsVariant;
    QStrign pattern;
    int start = -1;
    int end = -1;
    bool isValid = false;
};

typedef QStack<ParseToken> FrameStack;
