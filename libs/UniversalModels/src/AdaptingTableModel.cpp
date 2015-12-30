


#include "../include/AdaptingTableModel.h"
#include "../include/TableDataInterface.h"
#include "../include/AdaptingTableModelPrivate.h"
#include <QDebug>

AdaptingTableModel::AdaptingTableModel(QObject * parent) : QAbstractTableModel(parent),
d_ptr(new AdaptingTableModelPrivate())
{
    // Bouml preserved body begin 0020EEAA
    // Bouml preserved body end 0020EEAA
}

AdaptingTableModel::~AdaptingTableModel() 
{
    // Bouml preserved body begin 0020EF2A
    // Bouml preserved body end 0020EF2A
}

QVariant AdaptingTableModel::data(const QModelIndex & index, int role) const 
{
    // Bouml preserved body begin 0020EFAA
    Q_D(const AdaptingTableModel);
    if(!index.isValid())
        return QVariant();
    return d->interface->GetValue(index.row(),  index.column(), role);
    // Bouml preserved body end 0020EFAA
}

bool AdaptingTableModel::setData(const QModelIndex & index, const QVariant & value, int role) 
{
    // Bouml preserved body begin 0021172A
    Q_D(AdaptingTableModel);
    if(!index.isValid())
        return false;

    d->interface->SetValue(index.row(),  index.column(), role, value);
    emit dataChanged(index, index);
    return true;
    // Bouml preserved body end 0021172A
}

int AdaptingTableModel::rowCount(const QModelIndex & index) const 
{
    // Bouml preserved body begin 0020F02A
    Q_UNUSED(index);
    Q_D(const AdaptingTableModel);
    //qDebug() << "rowcount is: " << d->interface->rowCount();
    return d->interface->rowCount();
    // Bouml preserved body end 0020F02A
}

int AdaptingTableModel::columnCount(const QModelIndex & index) const 
{
    // Bouml preserved body begin 0020F0AA
    Q_UNUSED(index);
    Q_D(const AdaptingTableModel);
    return d->interface->columnCount();
    // Bouml preserved body end 0020F0AA
}

void AdaptingTableModel::SetInterface(QSharedPointer<TableDataInterface> _interface) 
{
    // Bouml preserved body begin 00218B2A
    Q_D(AdaptingTableModel);
    d->interface = _interface;
    TableDataInterface* interfacePointer = d->interface.data();
	disconnect(interfacePointer, SIGNAL(reloadData()), this, SLOT(OnReloadDataFromInterface()));
    disconnect(interfacePointer, SIGNAL(reloadDataForRow(int)), this, SLOT(OnReloadRowDataFromInterface(int)));
    connect(interfacePointer, SIGNAL(reloadData()), this, SLOT(OnReloadDataFromInterface()));
    connect(interfacePointer, SIGNAL(reloadDataForRow(int)), this, SLOT(OnReloadRowDataFromInterface(int)));
    // Bouml preserved body end 00218B2A
}

bool AdaptingTableModel::insertRow(int row, const QModelIndex & )
{
    // Bouml preserved body begin 00020DAB
    Q_D(AdaptingTableModel);
    if(row > rowCount())
        return false;
    beginInsertRows(QModelIndex(), row, row);
    d->interface->insertRow(row);
    endInsertRows();
    return true;
    // Bouml preserved body end 00020DAB
}

QModelIndex AdaptingTableModel::index(int row, int column, const QModelIndex & parent) const 
{
    // Bouml preserved body begin 00213CAA
	Q_D(const AdaptingTableModel);
    if (!hasIndex(row, column, parent))
        return QModelIndex();
    return createIndex(row, column, const_cast<void*>(d->interface->InternalPointer(row)));
    // Bouml preserved body end 00213CAA
}

Qt::ItemFlags AdaptingTableModel::flags(const QModelIndex & index) const 
{
    // Bouml preserved body begin 002170AA
    if (!index.isValid())
        return 0;
	Q_D(const AdaptingTableModel);
    Qt::ItemFlags result = d->interface->flags(index);
    return result;
    // Bouml preserved body end 002170AA
}

QVariant AdaptingTableModel::headerData(int section, Qt::Orientation orientation, int role) const 
{
    // Bouml preserved body begin 0021712A
    Q_D(const AdaptingTableModel);
    if(d->interface->rowCount() == 0 )
        return QVariant();
    if(section < 0 || section > d->interface->GetColumns().size())
        return QVariant();
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole)
    {
        //qDebug() << interface->GetColumns().at(section);
        return d->interface->GetColumns().at(section);
    }
    return QVariant();
    // Bouml preserved body end 0021712A
}

void AdaptingTableModel::ClearData() 
{
    // Bouml preserved body begin 000217AB
    Q_D(AdaptingTableModel);
    beginRemoveRows(QModelIndex(), 0, rowCount());
    d->interface->DropData();
    endRemoveRows();
    // Bouml preserved body end 000217AB
}

void AdaptingTableModel::updateAll() 
{
    // Bouml preserved body begin 000218AB
    emit dataChanged(index(0, 0, QModelIndex()), index(rowCount()-1, columnCount()-1,QModelIndex()));
    // Bouml preserved body end 000218AB
}

int AdaptingTableModel::RowForValue(void* value) 
{
    // Bouml preserved body begin 0002A62B
    Q_D(const AdaptingTableModel);
    for(int i(0); i < rowCount(QModelIndex()); ++i)
    {
        if(d->interface->Equal(i, value))
            return i;
    }
    return -1;
    // Bouml preserved body end 0002A62B
}

void AdaptingTableModel::sort() 
{
    // Bouml preserved body begin 0002A72B
    // Bouml preserved body end 0002A72B
}

void AdaptingTableModel::RemoveRow(const QModelIndex & index) 
{
    // Bouml preserved body begin 0002A6AB
    Q_D(AdaptingTableModel);
    if(!index.isValid())
        return;
    beginRemoveRows(QModelIndex(), index.row(),index.row());
    d->interface->RemoveRow(index.row());
    endRemoveRows();
    // Bouml preserved body end 0002A6AB
}

void AdaptingTableModel::OnReloadDataFromInterface() 
{
    // Bouml preserved body begin 0021702A
    Q_D(AdaptingTableModel);
    beginResetModel();
    qDebug() << "previous rowCount: " << d->interface->PreviousRowCount() ;
    if(d->interface->PreviousRowCount() != 0)
    {
        int removeLimit = d->interface->PreviousRowCount() == 0 ? 0 : d->interface->PreviousRowCount()- 1;
        qDebug() << "Removing rowCount: " << removeLimit;
        beginRemoveRows(QModelIndex(), 0, removeLimit);
        endRemoveRows();
    }
    endResetModel();
    beginResetModel();
    int lastRow = d->interface->rowCount() - 1;
    qDebug() << QString("New last row: ") << QString::number(lastRow);
    if(d->interface->rowCount() != 0)
    {
        beginInsertRows(QModelIndex(), 0, lastRow);
        endInsertRows();
    }
    endResetModel();
    // Bouml preserved body end 0021702A
}

void AdaptingTableModel::OnReloadRowDataFromInterface(int row) 
{
    // Bouml preserved body begin 0002142B
    emit dataChanged(index(row, 0,QModelIndex()), index(row, columnCount(),QModelIndex()));
    // Bouml preserved body end 0002142B
}

