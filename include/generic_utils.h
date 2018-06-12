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
