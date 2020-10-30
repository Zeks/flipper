#pragma once

namespace core {

class DBEntity{
public:
    bool HasChanges() const {return hasChanges;}
    virtual ~DBEntity(){}
    bool hasChanges = false;
};

enum class UpdateMode
{
    none = -1,
    insert = 0,
    update = 1
};
}
