#pragma once

#include <QString>
#include <memory>

namespace core{
namespace fandom_lists{

    enum EEntityType{
        et_list = 0,
        et_fandom = 1,
    };
    enum EInclusionMode{
        im_include = 0,
        im_exclude = 1,
    };
    struct ListBase{
        ListBase(EEntityType type):type(type){}
        bool isEnabled;
        uint32_t id;
        QString name;
        uint16_t uiIndex;
        EEntityType type;
        EInclusionMode inclusionMode;

    };

    struct List : public ListBase{
        List():ListBase(et_list){}
        using ListPtr = std::shared_ptr<List>;
        bool isDefault;
    };

    enum ECrossoverInclusionMode{
        cim_select_none = -1,
        cim_select_all = 0,
        cim_select_pure = 1,
        cim_select_crossovers = 2,
    };

    struct FandomStateInList: public ListBase{
        FandomStateInList():ListBase(et_fandom){}
        uint32_t list_id;
        ECrossoverInclusionMode crossoverInclusionMode;
    };

}
}
