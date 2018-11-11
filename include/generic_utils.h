/*
Flipper is a replacement search engine for fanfiction.net search results
Copyright (C) 2017-2018  Marchenko Nikolai

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
#include <QSharedPointer>
#include <functional>

template <typename T>
QList<QSharedPointer<T> >  ReverseSortedList(QList<QSharedPointer<T> >  list, std::function<bool(const T& i1, const T& i2)> comparator)
{
    qSort(list.begin(),list.end(), comparator);
    std::reverse(list.begin(),list.end());
    return list;
}

template <typename T>
QList<T> SortedList(T list)
{
    qSort(list.begin(),list.end());
    return list;
}
QString MicrosecondsToString(int value);
