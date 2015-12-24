#ifndef _ADAPTINGTABLEMODELPRIVATE_H
#define _ADAPTINGTABLEMODELPRIVATE_H


#include <QSharedPointer>

#include "l_tree_controller_global.h"

#include "AdaptingTableModel.h"
class AdaptingTableModel;
class TableDataInterface;


class   AdaptingTableModelPrivate  
{
Q_DECLARE_PUBLIC(AdaptingTableModel)
  public:
    explicit AdaptingTableModelPrivate();

    virtual ~AdaptingTableModelPrivate();


  private:
     AdaptingTableModel * q_ptr = nullptr;
    QSharedPointer<TableDataInterface> interface;

};
#endif
