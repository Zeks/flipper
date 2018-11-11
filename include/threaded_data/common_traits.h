#pragma once 
#include <QSharedPointer>
#include <QVector>
#include <QSet>
#include <QList>
#include <array>
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

    return has_serialize<typename T::value_type, QDataStream>();
};
