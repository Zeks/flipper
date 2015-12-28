#ifndef _ITEMCONTROLLER_H
#define _ITEMCONTROLLER_H


#include <QStringList>
#include <QVariant>
#include <QModelIndex>
#include <QVector>
#include <QHash>

#include "l_tree_controller_global.h"

#include <functional>

template<class T>
class ItemController  
{
  public:
    inline QStringList GetColumns() const;

    void SetColumns(QStringList value);

    QVariant GetValue(const T * item, int index, int role);

    bool SetValue(T * item, int column, const QVariant & value, int role);

    void AddGetter(const QPair<int,int> & index, std::function<QVariant(const T*)> function);

    void AddSetter(const QPair<int,int> & index, std::function<bool(T*, QVariant)> function);

    Qt::ItemFlags flags(const QModelIndex & index) const;

    static inline QVector<std::function<Qt::ItemFlags(const QModelIndex&)> > GetFlagsFunctors();

    static void SetFlagsFunctors(QVector<std::function<Qt::ItemFlags(const QModelIndex&)> > value);

    static void AddFlagsFunctor(std::function<Qt::ItemFlags(const QModelIndex&)> functor);

    static void SetDefaultTreeFunctor();


  private:
     QStringList columns;
     QHash<QPair<int, int>, std::function<QVariant(const T*)> > getters;
     QHash<QPair<int,int>, std::function<bool(T*, QVariant)> > setters;
     static QVector<std::function<Qt::ItemFlags(const QModelIndex&)> > flagsFunctors;
};
template<class T>
inline QStringList ItemController<T>::GetColumns() const 
{
    return columns;
}

template<class T>
void ItemController<T>::SetColumns(QStringList value) 
{
    columns = value;
}

template<class T>
QVariant ItemController<T>::GetValue(const T * item, int index, int role) 
{
    // Bouml preserved body begin 00203B2A
    //return QVariant();
    if(!getters.contains(QPair<int,int>(index, role)))
        return QVariant();
    QVariant ret = getters[QPair<int,int>(index, role)](item);
    return ret;
    // Bouml preserved body end 00203B2A
}

template<class T>
bool ItemController<T>::SetValue(T * item, int column, const QVariant & value, int role) 
{
    // Bouml preserved body begin 00203BAA
    if(!setters.contains(QPair<int,int>(column, role)))
        return false;
    return setters[QPair<int,int>(column, role)](item, value);
    // Bouml preserved body end 00203BAA
}

template<class T>
void ItemController<T>::AddGetter(const QPair<int,int> & index, std::function<QVariant(const T*)> function) 
{
    // Bouml preserved body begin 002117AA
    getters.insert(index, function);
    // Bouml preserved body end 002117AA
}

template<class T>
void ItemController<T>::AddSetter(const QPair<int,int> & index, std::function<bool(T*, QVariant)> function) 
{
    // Bouml preserved body begin 0021182A
    setters.insert(index, function);
    // Bouml preserved body end 0021182A
}

template<class T>
Qt::ItemFlags ItemController<T>::flags(const QModelIndex & index) const 
{
    // Bouml preserved body begin 0021AFAA
    Qt::ItemFlags result;
    for(auto func: flagsFunctors)
    {
        result |= func(index);
    }
    return result;
    // Bouml preserved body end 0021AFAA
}

template<class T>
inline QVector<std::function<Qt::ItemFlags(const QModelIndex&)> > ItemController<T>::GetFlagsFunctors()

{
    return flagsFunctors;
}

template<class T>
void ItemController<T>::SetFlagsFunctors(QVector<std::function<Qt::ItemFlags(const QModelIndex&)> > value)

{
    flagsFunctors = value;
}

template<class T>
void ItemController<T>::AddFlagsFunctor(std::function<Qt::ItemFlags(const QModelIndex&)> functor)

{
    // Bouml preserved body begin 0021B22A
    flagsFunctors.append(functor);
    // Bouml preserved body end 0021B22A
}

template<class T>
void ItemController<T>::SetDefaultTreeFunctor()

{
    // Bouml preserved body begin 0021B1AA
    AddFlagsFunctor([](const QModelIndex& index)
    {
        Qt::ItemFlags flags;
        if ( index.column() == 0 )
            flags |= Qt::ItemIsUserCheckable | Qt::ItemIsSelectable;
        return flags;
    }
    );
    // Bouml preserved body end 0021B1AA
}

template<class T>
QVector<std::function<Qt::ItemFlags(const QModelIndex&)> > ItemController<T>::flagsFunctors;
#endif
