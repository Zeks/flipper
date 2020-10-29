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
    struct ListBase : public std::enable_shared_from_this<ListBase>{
        ListBase(EEntityType type):type(type){}
        virtual ~ListBase(){};
        bool isEnabled;
        bool isExpanded;
        uint32_t id;
        QString name;
        uint16_t uiIndex;
        EEntityType type;
        EInclusionMode inclusionMode;
        EInclusionMode Rotate(EInclusionMode mode){
            if(mode == im_exclude)
                return im_include;
            return im_exclude;
        };

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
        using ListBase::Rotate;
        uint32_t list_id;
        ECrossoverInclusionMode crossoverInclusionMode;
        ECrossoverInclusionMode Rotate(ECrossoverInclusionMode mode){
            if(mode == cim_select_all)
                return cim_select_crossovers;
            if(mode == cim_select_crossovers)
                return cim_select_pure;
            return cim_select_all;
        };
    };

}
}
