#ifndef _TABLEDATALISTHOLDER_H
#define _TABLEDATALISTHOLDER_H


#include "TableDataInterface.h"
#include <QVariant>
#include <QStringList>
#include <QList>
#include <QModelIndex>
#include <QVector>
#include <QHash>
#include <QSharedPointer>
#include "ItemController.h"

#include "l_tree_controller_global.h"



template<class T>
class  TableDataListHolder : public TableDataInterface  
{
  public:
    virtual QVariant GetValue(int row, int column, int role) const;

    virtual void SetValue(int row, int column, int role, const QVariant & value);

    virtual QStringList GetColumns();

    void SetColumns(QStringList _columns);

    void SetData(const QList<T> & data);

    QList<T> GetData() const;

    QList<T>& GetData();

    virtual int columnCount() const;

    virtual int rowCount() const;

    void AddGetter(const QPair<int,int> & index, std::function<QVariant(const T*)> function);

    void AddSetter(const QPair<int,int> & index, std::function<void(T*, QVariant)> function);

    virtual int PreviousRowCount();

    bool insertRow(int row);

    void SetDataForRow(int row, const T & data);

    virtual void DropData();

    virtual const void* InternalPointer() const;

    virtual const void* InternalPointer(int row) const;

    virtual void* InternalPointer();

    virtual void* InternalPointer(int row);

    virtual bool Equal(int row, void* data);

    Qt::ItemFlags flags(const QModelIndex & index) const;

    virtual void sort();

    void AddFlagsFunctor(std::function<Qt::ItemFlags(const QModelIndex&)> functor);

    virtual void RemoveRow(int index);

    void SetFlagsFunctors(QVector<std::function<Qt::ItemFlags(const QModelIndex&)> > value);

    inline QVector<std::function<Qt::ItemFlags(const QModelIndex&)> > GetFlagsFunctors();

    inline std::function<bool(void*, void*)> GetSortFunction() const;

    void SetSortFunction(std::function<bool(void*, void*)> value);


  private:
     QList<T> m_data;
     QHash<QPair<int,int>, std::function<void(T*, QVariant)> > setters;
     QHash<QPair<int, int>, std::function<QVariant(const T*)> > getters;
    QSharedPointer<ItemController<T> > controller;

     QStringList m_columns;
     int previousRowCount = 0;
     QVector<std::function<Qt::ItemFlags(const QModelIndex&)> > flagsFunctors;
     std::function<bool(void*, void*)> sortFunction;
};
template<class T>
QVariant TableDataListHolder<T>::GetValue(int row, int column, int role) const 
{
    // Bouml preserved body begin 0021352A
    if(!getters.contains(QPair<int,int>(column, role)))
        return QVariant();
    QVariant ret = getters[QPair<int,int>(column, role)](&m_data.at(row));
    return ret;
    // Bouml preserved body end 0021352A
}

template<class T>
void TableDataListHolder<T>::SetValue(int row, int column, int role, const QVariant & value) 
{
    // Bouml preserved body begin 002135AA
    if(!setters.contains(QPair<int,int>(column, role)))
        return;
    setters[QPair<int,int>(column, role)](&m_data[row], value);
    // Bouml preserved body end 002135AA
}

template<class T>
QStringList TableDataListHolder<T>::GetColumns() 
{
    // Bouml preserved body begin 0021362A
    return m_columns;
    // Bouml preserved body end 0021362A
}

template<class T>
void TableDataListHolder<T>::SetColumns(QStringList _columns) 
{
    // Bouml preserved body begin 00213C2A
    m_columns = _columns;
    // Bouml preserved body end 00213C2A
}

template<class T>
void TableDataListHolder<T>::SetData(const QList<T> & data) 
{
    // Bouml preserved body begin 002136AA
    previousRowCount = m_data.size();
    m_data = data;
    emit reloadData();
    // Bouml preserved body end 002136AA
}

template<class T>
QList<T> TableDataListHolder<T>::GetData() const 
{
    // Bouml preserved body begin 00218C2A
    return m_data;
    // Bouml preserved body end 00218C2A
}

template<class T>
QList<T>& TableDataListHolder<T>::GetData() 
{
    // Bouml preserved body begin 0021E7AA
    return m_data;
    // Bouml preserved body end 0021E7AA
}

template<class T>
int TableDataListHolder<T>::columnCount() const 
{
    // Bouml preserved body begin 002138AA
    return m_columns.size();
    // Bouml preserved body end 002138AA
}

template<class T>
int TableDataListHolder<T>::rowCount() const 
{
    // Bouml preserved body begin 002137AA
    return m_data.size();
    // Bouml preserved body end 002137AA
}

template<class T>
void TableDataListHolder<T>::AddGetter(const QPair<int,int> & index, std::function<QVariant(const T*)> function) 
{
    // Bouml preserved body begin 00213A2A
    getters.insert(index, function);
    // Bouml preserved body end 00213A2A
}

template<class T>
void TableDataListHolder<T>::AddSetter(const QPair<int,int> & index, std::function<void(T*, QVariant)> function) 
{
    // Bouml preserved body begin 00213AAA
    setters.insert(index, function);
    // Bouml preserved body end 00213AAA
}

template<class T>
int TableDataListHolder<T>::PreviousRowCount() 
{
    // Bouml preserved body begin 0022372A
    return previousRowCount;
    // Bouml preserved body end 0022372A
}

template<class T>
bool TableDataListHolder<T>::insertRow(int row) 
{
    // Bouml preserved body begin 00020E2B
    if(row > rowCount())
        return false;
    m_data.insert(m_data.begin() + row, T());
    return true;
    // Bouml preserved body end 00020E2B
}

template<class T>
void TableDataListHolder<T>::SetDataForRow(int row, const T & data) 
{
    // Bouml preserved body begin 0002112B
    if(m_data.size() >= row+1)
        m_data[row] = data;
    emit reloadDataForRow(row);
    // Bouml preserved body end 0002112B
}

template<class T>
void TableDataListHolder<T>::DropData() 
{
    // Bouml preserved body begin 000215AB
    m_data.clear();
    // Bouml preserved body end 000215AB
}

template<class T>
const void* TableDataListHolder<T>::InternalPointer() const
{
    // Bouml preserved body begin 0002AE2B
    if(m_data.size() == 0)
        return 0;
    return const_cast<const T*>(&m_data[0]);
    // Bouml preserved body end 0002AE2B
}

template<class T>
const void* TableDataListHolder<T>::InternalPointer(int row) const
{
    // Bouml preserved body begin 0002ADAB
    if(m_data.size() < row)
        return 0;
    return const_cast<const T*>(&m_data[row]);
    // Bouml preserved body end 0002ADAB
}

template<class T>
void* TableDataListHolder<T>::InternalPointer() 
{
    // Bouml preserved body begin 0002AFAB
    if(m_data.size() == 0)
        return 0;
    return &m_data[0];
    // Bouml preserved body end 0002AFAB
}

template<class T>
void* TableDataListHolder<T>::InternalPointer(int row) 
{
    // Bouml preserved body begin 0002B02B
    if(m_data.size() < row)
        return 0;
    return &m_data[row];
    // Bouml preserved body end 0002B02B
}

template<class T>
bool TableDataListHolder<T>::Equal(int , void* )
{
    // Bouml preserved body begin 0002AD2B
    return false;
    // Bouml preserved body end 0002AD2B
}

template<class T>
Qt::ItemFlags TableDataListHolder<T>::flags(const QModelIndex & index) const 
{
    // Bouml preserved body begin 0002ACAB
    Qt::ItemFlags result;
    for(auto& func: flagsFunctors)
    {
        result |= func(index);
    }
    return result;
    // Bouml preserved body end 0002ACAB
}

template<class T>
void TableDataListHolder<T>::sort() 
{
    // Bouml preserved body begin 0002AC2B
    //qSort(m_data.begin(), m_data.end(), sortFunction);
    // Bouml preserved body end 0002AC2B
}

template<class T>
void TableDataListHolder<T>::AddFlagsFunctor(std::function<Qt::ItemFlags(const QModelIndex&)> functor) 
{
    // Bouml preserved body begin 0002CAAB
	flagsFunctors.append(functor);
    // Bouml preserved body end 0002CAAB
}

template<class T>
void TableDataListHolder<T>::RemoveRow(int index) 
{
    // Bouml preserved body begin 0002ABAB
    m_data.erase(m_data.begin()+index);
    // Bouml preserved body end 0002ABAB
}

template<class T>
void TableDataListHolder<T>::SetFlagsFunctors(QVector<std::function<Qt::ItemFlags(const QModelIndex&)> > value) 
{
    flagsFunctors = value;
}

template<class T>
inline QVector<std::function<Qt::ItemFlags(const QModelIndex&)> > TableDataListHolder<T>::GetFlagsFunctors() 
{
    return flagsFunctors;
}

template<class T>
inline std::function<bool(void*, void*)> TableDataListHolder<T>::GetSortFunction() const 
{
    return sortFunction;
}

template<class T>
void TableDataListHolder<T>::SetSortFunction(std::function<bool(void*, void*)> value) 
{
    sortFunction = value;
}

#endif
