#ifndef _ADDRESSTEMPLATEDATA_H
#define _ADDRESSTEMPLATEDATA_H


#include "InterfaceAwareTreeData.h"
#include <QString>
#include <QDebug>
#include "AddressTemplateTreeData.h"
#include <QVector>

#include "l_tree_controller_global.h"



class L_TREE_CONTROLLER_EXPORT AddressTemplateData : public InterfaceAwareTreeData  
{
  public:
    explicit AddressTemplateData();

    virtual ~AddressTemplateData();

    enum EAddressType 
    {
          eat_not_applicable = -1,
      eat_adp = 0,
      eat_rc = 1,
      eat_pvo = 4,
      eat_mil_callsign = 5,
      eat_user_civic = 8,
      eat_user_military = 7,
      eat_gp = 2,
      eat_mdp = 3

    };

typedef AddressTemplateData value_type;    void SetAddressType(EAddressType value);

    inline EAddressType GetAddressType() const;

    inline bool GetAcceptsNew() const;

    void SetAcceptsNew(bool value);

    inline QString GetAddress() const;

    void SetAddress(QString value);

    inline QString GetName() const;

    void SetName(QString value);

    virtual void* GetParent();

    inline AddressTemplateTreeData& GetTreeData();

    void SetTreeData(AddressTemplateTreeData value);

    virtual QVector<void*> GetChildren();

    virtual AddressTemplateData* CastPointer(void * pointer);

    inline bool GetIsLinkedToAddressTable() const;

    void SetIsLinkedToAddressTable(bool value);

    bool HasValidAddress();


  private:
     bool acceptsNew;
     QString name;
     QString address;
     EAddressType addressType;
    AddressTemplateTreeData treeData;

     bool isLinkedToAddressTable;
};
inline AddressTemplateData::EAddressType AddressTemplateData::GetAddressType() const 
{
    return addressType;
}

inline bool AddressTemplateData::GetAcceptsNew() const 
{
    return acceptsNew;
}

inline QString AddressTemplateData::GetAddress() const 
{
    return address;
}

inline QString AddressTemplateData::GetName() const 
{
    return name;
}

inline AddressTemplateTreeData& AddressTemplateData::GetTreeData() 
{
    return treeData;
}

inline bool AddressTemplateData::GetIsLinkedToAddressTable() const 
{
    return isLinkedToAddressTable;
}

#endif
