#ifndef _ADDRESSTEMPLATETREEDATA_H
#define _ADDRESSTEMPLATETREEDATA_H


#include <QString>
#include <QDebug>

#include "l_tree_controller_global.h"



class L_TREE_CONTROLLER_EXPORT AddressTemplateTreeData  
{
  public:
    explicit AddressTemplateTreeData();

    virtual ~AddressTemplateTreeData();

    enum class ENodeType 
    {
      nt_folder = 0,
      nt_node = 1

    };

    inline int GetAddressColumn() const;

    void SetAddressColumn(int value);

    inline int GetId() const;

    void SetId(int value);

    inline int GetPar_id() const;

    void SetPar_id(int value);

    inline QString GetCode() const;

    void SetCode(QString value);

    inline bool GetIsEnabled() const;

    void SetIsEnabled(bool value);

    inline ENodeType GetNodeType() const;

    void SetNodeType(ENodeType value);


  private:
     int par_id;
     int addressColumn = -1;
     int id;
     QString code;
     bool isEnabled;
     ENodeType nodeType;
};
inline int AddressTemplateTreeData::GetAddressColumn() const 
{
    return addressColumn;
}

inline int AddressTemplateTreeData::GetId() const 
{
    return id;
}

inline int AddressTemplateTreeData::GetPar_id() const 
{
    return par_id;
}

inline QString AddressTemplateTreeData::GetCode() const 
{
    return code;
}

inline bool AddressTemplateTreeData::GetIsEnabled() const 
{
    return isEnabled;
}

inline AddressTemplateTreeData::ENodeType AddressTemplateTreeData::GetNodeType() const 
{
    return nodeType;
}

#endif
