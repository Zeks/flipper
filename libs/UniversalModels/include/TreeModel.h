#ifndef RXTREE_TREEMODEL_H
#define RXTREE_TREEMODEL_H


#include <QObject>
#include <QVariant>
#include <QModelIndex>
#include <QString>
#include <QDebug>
#include "include/TreeItemInterface.h"
#include "l_tree_controller_global.h"
class QTreeView;

class L_TREE_CONTROLLER_EXPORT TreeModel  : public QAbstractItemModel
{
  Q_OBJECT
  public:
    TreeModel(QObject * parent = 0);

    virtual ~TreeModel();

    QVariant data(const QModelIndex & index, int role) const;

    bool setData(const QModelIndex & index, const QVariant & value, int role = Qt::EditRole );

    Qt::ItemFlags flags(const QModelIndex & index) const;

    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;

    QModelIndex index(int row, int column, const QModelIndex & parent = QModelIndex()) const;

    QModelIndex parent(const QModelIndex & index) const;

    int rowCount(const QModelIndex & parent = QModelIndex()) const;

    int columnCount(const QModelIndex & parent = QModelIndex()) const;

    TreeItemInterface * RootItem();

    void InsertRootItem(QSharedPointer<TreeItemInterface>);

    TreeItemInterface * getItem(const QModelIndex & index) const;

    bool insertRows(int position, int rows, const QModelIndex & parent);

    bool removeRows(int position, int rows, const QModelIndex & parent);

    void reset();

    //void dataChanged(const QModelIndex&, const QModelIndex&);
    void Refresh();

    void AddRoleFunc(int role, std::function<QVariant(const QModelIndex & index)> func);

    void AddDefaultRoleFunc(std::function<QVariant (const QModelIndex & index, int role)> func);

    QSharedPointer<TreeItemInterface> rootItem;



signals:
    void dataEditFinished(const QModelIndex &);
    void itemCheckStatusChanged(const QModelIndex&);
private:
    QHash<uint, std::function<QVariant(const QModelIndex & index)>> roleFuncs;
    std::function<QVariant(const QModelIndex & index, int role)> defaultRoleFunc;



};


#endif
