


#include "../include/InterfaceAwareTreeData.h"
#include "../include/TreeItemInterface.h"

void* InterfaceAwareTreeData::GetParent() 
{
    // Bouml preserved body begin 0020A0AA
    return static_cast<void*>(interface->parent());
    // Bouml preserved body end 0020A0AA
}

QVector<void*> InterfaceAwareTreeData::GetChildren() 
{
    // Bouml preserved body begin 0020A12A
    QVector<void*> test;
    return test;
    // Bouml preserved body end 0020A12A
}

void InterfaceAwareTreeData::SetInterfacePointer(TreeItemInterface * pointer) 
{
    // Bouml preserved body begin 0020A1AA
    interface = pointer;
    // Bouml preserved body end 0020A1AA
}

InterfaceAwareTreeData* InterfaceAwareTreeData::CastPointer(void * pointer) 
{
    // Bouml preserved body begin 0020D1AA
    return static_cast<InterfaceAwareTreeData*>(pointer);
    // Bouml preserved body end 0020D1AA
}

