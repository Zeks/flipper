


#include "../include/AddressTemplateData.h"

AddressTemplateData::AddressTemplateData() 
{
    // Bouml preserved body begin 0020D4AA
    acceptsNew = false;
    name = QString();
    address = QString();
    addressType = eat_user_civic;
    isLinkedToAddressTable = false;
    // Bouml preserved body end 0020D4AA
}

AddressTemplateData::~AddressTemplateData() 
{
    // Bouml preserved body begin 0020D52A
    // Bouml preserved body end 0020D52A
}

void AddressTemplateData::SetAddressType(AddressTemplateData::EAddressType value) 
{
    addressType = value;
}

void AddressTemplateData::SetAcceptsNew(bool value) 
{
    acceptsNew = value;
}

void AddressTemplateData::SetAddress(QString value) 
{
    address = value;
}

void AddressTemplateData::SetName(QString value) 
{
    name = value;
}

void* AddressTemplateData::GetParent() 
{
    // Bouml preserved body begin 0020D22A
    return 0;
    // Bouml preserved body end 0020D22A
}

void AddressTemplateData::SetTreeData(AddressTemplateTreeData value) 
{
    treeData = value;
}

QVector<void*> AddressTemplateData::GetChildren() 
{
    // Bouml preserved body begin 0020D2AA
    return QVector<void*>();
    // Bouml preserved body end 0020D2AA
}

AddressTemplateData* AddressTemplateData::CastPointer(void * pointer) 
{
    // Bouml preserved body begin 0020D32A
    return static_cast<AddressTemplateData*>(pointer);
    // Bouml preserved body end 0020D32A
}

void AddressTemplateData::SetIsLinkedToAddressTable(bool value) 
{
    isLinkedToAddressTable = value;
}

bool AddressTemplateData::HasValidAddress() 
{
    // Bouml preserved body begin 0021AF2A
    return !address.trimmed().isEmpty();
    // Bouml preserved body end 0021AF2A
}

