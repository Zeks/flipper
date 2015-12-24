#ifndef _INTERFACEAWARETREEDATA_H
#define _INTERFACEAWARETREEDATA_H


#include <QVector>

#include "l_tree_controller_global.h"


class TreeItemInterface;


class L_TREE_CONTROLLER_EXPORT InterfaceAwareTreeData
{
  private:
    TreeItemInterface * interface;


  public:
    virtual void* GetParent();

    virtual QVector<void*> GetChildren();

    void SetInterfacePointer(TreeItemInterface * pointer);

    virtual InterfaceAwareTreeData* CastPointer(void * pointer);

};
#endif
