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
#include <QString>
#include <QDateTime>
#include <QByteArray>
#include <QScopedPointer>
#include <QSqlDatabase>
#include <QThread>
#include "ECacheMode.h"
#include <atomic>
enum class EPageType
{
    hub_page = 0,
    sorted_ficlist = 1,
    author_profile = 2,
    fic_page = 3
};

enum class EPageSource
{
    none = -1,
    network = 0,
    cache = 1,
};

struct WebPage
{
    friend class PageThreadWorker;
    QString url;
    QDateTime generated;
    QDate minFicDate;
    //QString stringContent;
    QString content;
    QString previousUrl;
    QString nextUrl;
    QStringList referencedFics;
    QString fandom;
    bool crossover;
    EPageType type;
    bool isValid = false;
    bool failedToAcquire = false;
    EPageSource source = EPageSource::none;
    QString error;
    QString comment;
    bool isLastPage = false;
    bool isFromCache = false;
    int pageIndex = 0;
    int id = -1;
    QString LoadedIn() {
        QString decimal = QString::number(loadedIn/1000000);
        int offset = decimal == "0" ? 0 : decimal.length();
        QString partial = QString::number(loadedIn).mid(offset,1);
        return decimal + "." + partial;}
private:
    int loadedIn = 0;
};
