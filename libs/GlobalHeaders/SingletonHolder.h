/*
 * SingletonHolder.h
 *
 *  Created on: 07.09.2011
 *      Author: Admin
 */

#ifndef SINGLETONHOLDER1_H_
#define SINGLETONHOLDER1_H_
#include <stdexcept>
#include <typeinfo>
#include <memory>
#include <QSharedMemory>
#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include <QSettings>

template<typename T>
//__attribute__((visibility("default")))
struct  An
{
    template<typename U>
    friend struct An;

    An()                              {}

    template<typename U>
    An(const An<U>& a) : data(a.data) {}

    template<typename U>
    An(An<U>&& a) : data(std::move(a.data)) {}

    T* operator->()
     {
        //qDebug() << "in operator ->";
        return get0();
    }
    const T* operator->() const
    {
        //qDebug() << "in const operator ->";
        return get0();
    }
    bool isEmpty() const
    {
        return !data;
    }
    void clear()
    {
        //qDebug() << QString("Calling clear from signleton");
        localData.reset(); data = nullptr;
        QSettings idFile("Settings/ParserSettings.ini", QSettings::IniFormat);
        int modValue = idFile.value("Local/modValue1").toInt();
#ifdef Q_OS_UNIX
        QString userName = qgetenv("USER");
        if(userName.isEmpty())
            userName = qgetenv("USERNAME");
        QString segmentName =  userName + QString("MEMSEG") + QCoreApplication::applicationName() + QString(typeid(data).name());
#endif
#ifndef Q_OS_UNIX
        QString segmentName =   QString("MEMSEG") + QCoreApplication::applicationName() + QString(typeid(data).name());
#endif
        //qDebug() << QString("segment name is: ") << segmentName;
        QSharedMemory memory(segmentName);
        bool memoryAttached = memory.attach(QSharedMemory::ReadWrite);
        //qDebug() << QString("memory attach status: ") << memoryAttached;
        if(memoryAttached)
        {
            memory.lock();
            char* to = (char*)memory.data();
            intptr_t address = 0;
            memcpy(to, &address, sizeof(intptr_t));
            memory.unlock();
        }


    }
    void init()
    {
        //qDebug() << "in init func";
        if (!data)
            reinit();
    }
    void reinit()
    {
        //qDebug() << "in reinit func";
        anFill(*this);
    }

    T& create()
    {
        //qDebug() << "in create func";
        return create<T>();
    }

    template<typename U>
    U& create()
    {
        U* u = nullptr;

#ifdef Q_OS_UNIX
        QString userName = qgetenv("USER");
        if(userName.isEmpty())
            userName = qgetenv("USERNAME");
        QString segmentName =  userName + QString("MEMSEG") + QCoreApplication::applicationName() + QString(typeid(data).name());
#endif
#ifndef Q_OS_UNIX
        QString segmentName =   QString("MEMSEG") + QCoreApplication::applicationName() + QString(typeid(data).name());
#endif
        //qDebug() << "App name is: " << QCoreApplication::applicationName();
        //qDebug() << QString("segment name is: ") << segmentName;
        static QSharedMemory memory(segmentName);
        memory.setKey(segmentName);
        if (memory.attach(QSharedMemory::ReadWrite))
        {
            qDebug() << segmentName << " detaching segment";
            memory.detach();
            memory.attach(QSharedMemory::ReadWrite);
            memory.detach();
        }
        bool memoryAttached = memory.attach(QSharedMemory::ReadWrite);
        //qDebug() << "memory attach status: " << memoryAttached;
        if(!memoryAttached)
        {
            //qDebug() << "memory was not attached, creating new " + segmentName;
            /*QLOG_FATAL() << "segment not attached";
            QLOG_FATAL() << memory.errorString();
            qxtLog->critical("trying to create memory segment");*/
            int dataSize = sizeof(u);
            bool created = memory.create(dataSize, QSharedMemory::ReadWrite);
            if(!created)
            {
                qDebug() << segmentName << " segment not created";
                qDebug() << memory.errorString();
            }
            u = new U;
            localData.reset(u);
            data = u;
            memory.lock();
            char* to = (char*)memory.data();
            intptr_t address = reinterpret_cast<intptr_t>(reinterpret_cast<intptr_t*>(u));
            //qDebug() << "memory address: " << address;
            memcpy(to, &address, sizeof(intptr_t));
            memory.unlock();
            return *u;
        }

        QString errorstring = memory.errorString();
        if(errorstring.trimmed().isEmpty())
        {
            //qDebug() << "memory was attached, picking " + segmentName;
            memory.lock();
            //qDebug() << memory.errorString();
            char* from = (char*)memory.data();
            //char to[sizeof(intptr_t)];
            intptr_t address;
            memcpy(&address, from, sizeof(intptr_t));
            //qDebug() << "memory address: " << address;
            if(address == 0)
            {
                u = new U;
                localData.reset(u);
                data = u;
                char* to = (char*)memory.data();
                intptr_t address = reinterpret_cast<intptr_t>(reinterpret_cast<int*>(u));
                //qDebug() << "memory address: " << address;
                memcpy(to, &address, sizeof(intptr_t));
                memory.unlock();
                return *u;

            }
            else
            {
                u = (U*)address;
            }
            memory.unlock();
                        data = u;
                        return *u;
        }
                else
                {
                        throw std::logic_error("shared memory error");
                }
    }

    template<typename U>
    void produce(U&& u)               { anProduce(*this, u); }

    template<typename U>
    void copy(const An<U>& a)         { data.reset(new U(*a.data)); }

    T* getData()
    {
        //qDebug() << "in getdata func";
        const_cast<An*>(this)->init();
        return data;
    }
    std::shared_ptr<T> getSharedData(){return localData;}
    void setSharedData(std::shared_ptr<T> _data){data = _data;}

private:
    T* get0() const
    {
        //qDebug() << "in get0 func";
        const_cast<An*>(this)->init();
        //qDebug() << typeid(T).name() << " " << data << " " << localData.get();
        return data;
    }

    T* data = nullptr;
    std::shared_ptr<T> localData;
};
template<typename T>
inline void anFill(An<T>& )
{
    //qDebug() << "failed creating singleton object";
    throw std::runtime_error(std::string("Cannot find implementation for interface: ")
            + typeid(T).name());
}
template<typename T>
struct AnAutoCreate : An<T>
{
    AnAutoCreate()
    {
        //qDebug() << "in create constructor";
        An<T>::create();
    }
};


template<typename T> inline
T& single()
{
    static T t;
    //qDebug() << typeid(T).name() << " " << &t;
    return t;
}

template<typename T> inline
An<T> anSingle()
{
    //qDebug() << "in ansingle constructor";
    return single<AnAutoCreate<T> >();
}

#define PROTO_IFACE(D_iface, D_an)              template<>  inline void anFill<D_iface>(An<D_iface>& D_an)

#define DECLARE_IMPL(D_iface)                   PROTO_IFACE(D_iface, a);

#define BIND_TO_IMPL_SINGLE(D_iface, D_impl)    PROTO_IFACE(D_iface, a) { a = anSingle<D_impl>(); }

#define BIND_TO_SELF_SINGLE(D_impl)             BIND_TO_IMPL_SINGLE(D_impl, D_impl)

#endif /* SINGLETONHOLDER1_H_ */
