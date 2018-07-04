#pragma once
#include <stdexcept>
#include <typeinfo>
#include <memory>
#include <mutex>
#include <functional>
#include <QSharedPointer>
template<typename T>
struct An
{
    template<typename U>
    friend struct An;

    An()                              {}

    template<typename U>
    An(const An<U>& a) : data(a.data), cleanupFunctor(a.cleanupFunctor) {}

    template<typename U>
    An(An<U>&& a) : data(std::move(a.data)), cleanupFunctor(std::move(a.cleanupFunctor)) {}

    T* operator->() {
        return getData();
    }
    const T* operator->() const {
        return getData();
    }

    bool isEmpty() const {
        return !data;
    }
    void clear() {
        data.reset();
    }
    void init() {
        if (!data)
            reinit();
    }
    void reinit() {
        anFill(*this);
    }

    T& create() {
        return create<T>();
    }

    template<typename U>
    U& create() {
        U* u = new U;
        data.reset(u);
        return *u; }

    template<typename U>
    void produce(U&& u) {
        anProduce(*this, u);
    }

    template<typename U>
    void copy(const An<U>& a) {
        data.reset(new U(*a.data));
    }

    T* getData() const
    {
        const_cast<An*>(this)->init();
        return data.get();
    }
    void FullDestroy(){
        init();
        clear();
        cleanupFunctor();
    }
protected:
    std::function<void()> cleanupFunctor;
    std::shared_ptr<T> data;
};

template<typename T>
struct AnAutoCreate : An<T>
{
    using An<T>::create;
    AnAutoCreate()
    {
        create();
        this->cleanupFunctor = std::bind(&AnAutoCreate::clear, this);
    }
};

template<typename T>
T& single()
{
    static T t;
    return t;
}

template<typename T>
An<T> anSingle()
{
    return single<AnAutoCreate<T>>();
}

template<typename T>
inline void anFill(An<T>& )
{
    //qDebug() << "failed creating singleton object";
    throw std::runtime_error(std::string("Cannot find implementation for interface: ")
                             + typeid(T).name());
}

#define PROTO_IFACE(D_iface, D_an)              template<>  inline void anFill<D_iface>(An<D_iface>& D_an)

#define DECLARE_IMPL(D_iface)                   PROTO_IFACE(D_iface, a);

#define BIND_TO_IMPL_SINGLE(D_iface, D_impl)    PROTO_IFACE(D_iface, a) { a = anSingle<D_impl>(); }

#define BIND_TO_SELF_SINGLE(D_impl)             BIND_TO_IMPL_SINGLE(D_impl, D_impl)


//BIND_TO_SELF_SINGLE(TypeB)
//BIND_TO_IMPL_SINGLE(TypeB, TypeB)
//PROTO_IFACE(TypeB, a) { a = anSingle<TypeB>(); }

//template<>  inline void anFill<TypeB>(An<TypeB>& a)
//{ a = anSingle<TypeB>(); }


