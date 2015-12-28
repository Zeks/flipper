#ifndef SIGNALBLOCKERWRAPPER_H
#define SIGNALBLOCKERWRAPPER_H


#include <stdexcept>
#include <typeinfo>
#include <QObject>
#include <QCoreApplication>
#include <functional>

template<class T>
class QSignalBlockerCallProxy
{
    T * const o;
public:
    explicit QSignalBlockerCallProxy( T * oo )
      : o(oo)
    {
    }
    T* operator->()
    {
         return o;
    }
    ~QSignalBlockerCallProxy()
    {
        if(o)
            o->blockSignals(false);
    }
};

template<class T>
class SignalBlocker
{
    T * const o;
public:
    explicit SignalBlocker( T * oo )
      : o(oo)
    {
    }
    QSignalBlockerCallProxy<T> operator->()
    {
         if (o)
             o->blockSignals( true );
         return QSignalBlockerCallProxy<T>(o);
    }
    ~SignalBlocker() {}
};

template<class T>
SignalBlocker<T> SilentCall(T* o)
{
    return SignalBlocker<T>(o);
}

template<class T>
void SilentCall(T* widget, std::function<void()> f)
{
    widget->blockSignals(true);
    f();
    widget->blockSignals(false);
}
//template <typename T>
struct NoSignalScope
{
    NoSignalScope(QObject* object, std::function<void()> f):objects({object})
    {
        for(auto& obj : objects)
        {
            obj->blockSignals(true);
        }
        f();
    }

    NoSignalScope(QList<QObject*> silentObjects, std::function<void()> f):objects(silentObjects)
    {
        for(auto& obj : objects)
        {
            obj->blockSignals(true);
        }
        f();
    }

    QList<QObject*> objects;
    ~NoSignalScope()
    {
        for(auto& obj : objects)
        {
            obj->blockSignals(false);
        }
    }
};
//template <typename T1, typename T2>
struct NoSignalScope2
{
    NoSignalScope2(QObject* object, QObject* secondObject, std::function<void()> f):obj(object), secondObj(secondObject)
    {
        obj->blockSignals(true);
        secondObj->blockSignals(true);
        f();
    }
    QObject* obj;
    QObject* secondObj;
    ~NoSignalScope2(){obj->blockSignals(false);secondObj->blockSignals(false);}
};

struct ScopeLocker
{
    ScopeLocker(bool& value, std::function<void()> f):condition{value}
    {
        condition = true;
        f();
    }

    bool& condition;
    ~ScopeLocker()
    {
        condition = false;
    }
};

#endif // SIGNALBLOCKERWRAPPER_H
