


#include "../include/AddressTemplateTreeData.h"

AddressTemplateTreeData::AddressTemplateTreeData() 
{
    // Bouml preserved body begin 0021B32A
    par_id = -1;
    addressColumn = -1;
    id = -1;
    isEnabled = false;
    nodeType = ENodeType::nt_folder;
    // Bouml preserved body end 0021B32A
}

AddressTemplateTreeData::~AddressTemplateTreeData() 
{
    // Bouml preserved body begin 0021B3AA
    // Bouml preserved body end 0021B3AA
}

void AddressTemplateTreeData::SetAddressColumn(int value) 
{
    addressColumn = value;
}

void AddressTemplateTreeData::SetId(int value) 
{
    id = value;
}

void AddressTemplateTreeData::SetPar_id(int value) 
{
    par_id = value;
}

void AddressTemplateTreeData::SetCode(QString value) 
{
    code = value;
}

void AddressTemplateTreeData::SetIsEnabled(bool value) 
{
    isEnabled = value;
}

void AddressTemplateTreeData::SetNodeType(AddressTemplateTreeData::ENodeType value) 
{
    nodeType = value;
}

