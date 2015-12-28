#ifndef _TABLEDATAINTERFACE_H
#define _TABLEDATAINTERFACE_H


#include <QObject>
#include <QVariant>
#include <QStringList>
#include <QModelIndex>
#include <functional>

#include "l_tree_controller_global.h"



class L_TREE_CONTROLLER_EXPORT TableDataInterface  : public QObject
{

Q_OBJECT

  public:
    TableDataInterface(QObject * parent = 0);

    virtual ~TableDataInterface();

    virtual QVariant GetValue(int row, int column, int role) const = 0;

    virtual void SetValue(int row, int column, int role, const QVariant & value) = 0;

    virtual QStringList GetColumns() = 0;

    virtual int rowCount() const = 0;

    virtual int columnCount() const = 0;

    virtual int PreviousRowCount() = 0;

    virtual bool insertRow(int row) = 0;

    virtual void DropData() = 0;

    virtual const void* InternalPointer() const = 0;

    virtual const void* InternalPointer(int row) const = 0;

    virtual bool Equal(int row, void* data) = 0;

    virtual void SetSortFunction(std::function<bool(void*, void*)> func) = 0;

    virtual void sort() = 0;

    virtual Qt::ItemFlags flags(const QModelIndex & index) const = 0;

    virtual void RemoveRow(int index) = 0;


signals:
    void reloadData();

    void reloadDataForRow(int row);

};
#endif
