#ifndef _ADAPTINGTABLEMODEL_H
#define _ADAPTINGTABLEMODEL_H


#include <QObject>
#include <QVariant>
#include <QModelIndex>
#include <QSharedPointer>

#include "l_tree_controller_global.h"

#include <QAbstractTableModel>

class TableDataInterface;
class AdaptingTableModelPrivate;


class L_TREE_CONTROLLER_EXPORT AdaptingTableModel  : public QAbstractTableModel
{

Q_OBJECT

  public:
    AdaptingTableModel(QObject * parent = 0);

    virtual ~AdaptingTableModel();

    QVariant data(const QModelIndex & index, int role) const;

    bool setData(const QModelIndex & index, const QVariant & value, int role);

    int rowCount(const QModelIndex & index = QModelIndex()) const;

    int columnCount(const QModelIndex & index = QModelIndex()) const;

    void SetInterface(QSharedPointer<TableDataInterface> _interface);

    bool insertRow(int row, const QModelIndex & parent);

    QModelIndex index(int row, int column, const QModelIndex & parent = QModelIndex()) const;

    Qt::ItemFlags flags(const QModelIndex & index) const;

    QVariant headerData(int section, Qt::Orientation orientation, int role) const;

    void ClearData();

    void updateAll();

    int RowForValue(void* value);

    void sort();

    void RemoveRow(const QModelIndex & index);


public slots:
    void OnReloadDataFromInterface();

    void OnReloadRowDataFromInterface(int row);


  private:


Q_DECLARE_PRIVATE(AdaptingTableModel)
  protected:
     AdaptingTableModelPrivate* const d_ptr;
};
#endif
