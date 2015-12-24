#ifndef RANDOMINSANITY_H
#define RANDOMINSANITY_H


template<typename ContainerType>
void TrimEverything(ContainerType& container)
{
    for(auto& val: container)
    {
        val = val.trimmed();
    }
}


template<typename>
struct IsContainer : public std::false_type{};


template <typename T, typename L, template <typename L> class K> struct
        IsContainer <QHash<T,K<L>>> : public std::true_type {};

template<typename>
struct IsQSmartPointer : public std::false_type{};
template<typename ValueType>
struct IsQSmartPointer<QSharedPointer<ValueType>> : public std::true_type{};
template<typename ValueType>
struct IsQSmartPointer<QScopedPointer<ValueType>> : public std::true_type{};


template<typename DataVecType,
         typename KeyType, typename ValueType,
         template <typename ,typename> class HashType,
         typename BitAccessFunc,
         typename = typename std::enable_if<IsContainer<HashType<KeyType, ValueType>>::value>::type,
         typename = typename std::enable_if<IsQSmartPointer<ValueType>::value>::type>
bool TrueIf(DataVecType data, HashType<KeyType, ValueType> hash, BitAccessFunc baf)
{
    bool valid = true;
    for(auto dataBit : data)
    {
        auto v = hash[dataBit.trimmed()].data();
        auto bitAccessor = std::bind(baf, v);
        if(!bitAccessor())
            valid = false;
    }
    return valid;
}


namespace Convert{
template<typename T>
struct memfun_type
{
    using type = void;
};

template<typename Ret, typename Class, typename... Args>
struct memfun_type<Ret(Class::*)(Args...) const>
{
    using type = std::function<Ret(Args...)>;
};

template<typename F>
typename memfun_type<decltype(&F::operator())>::type
function(F const &func)
{ // Function from lambda !
    return func;
}
}


#endif
