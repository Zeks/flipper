/*
Flipper is a replacement search engine for fanfiction.net search results
Copyright (C) 2017  Marchenko Nikolai

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
#include <QVector>
#include <QSet>
#include <QList>
#include <array>
#include "third_party/roaring/roaring.hh"
template <typename T>
struct is_shared_ptr {
    static const bool value = false;
};

template <typename Key>
struct is_shared_ptr<QSharedPointer<Key> > {
    static const bool value = true;
};

template <typename T>
struct is_hash {
    static const bool value = false;
};

template <typename Key,typename Value>
struct is_hash<QHash<Key,Value> > {
    static const bool value = true;
};

template <typename T>
struct is_vector{
    static const bool value = false;
};

template <typename Key>
struct is_vector<QVector<Key> > {
    static const bool value = true;
};


template <typename T>
struct is_roaring{
    static const bool value = false;
};

template <>
struct is_roaring<Roaring> {
    static const bool value = true;
};

template <typename Key>
struct is_vector<QList<Key> > {
    static const bool value = true;
};

namespace detail{
template<class> struct sfinae_true : std::true_type{};

template<class T, class A0>
static auto test_serialize(int)
-> sfinae_true<decltype(std::declval<T>().Serialize(std::declval<A0>()))>;
template<class, class A0>
static auto test_serialize(long) -> std::false_type;
} // detail::

template<class T, class Arg>
struct has_serialize: decltype(detail::test_serialize<T, Arg>(0)){};

template <typename T>
constexpr bool is_serializable_pointer (){

    if constexpr(!is_shared_ptr<T>::value)
            return false;
    constexpr bool is = has_serialize<typename T::value_type, QDataStream&>();
    return is;
};



