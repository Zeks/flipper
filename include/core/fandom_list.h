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
        im_exclude = 0,
        im_include = 1,
    };
    struct ListBase : public std::enable_shared_from_this<ListBase>{
        ListBase(EEntityType type):type(type){}
        std::shared_ptr<ListBase> getShared(){return shared_from_this();};
        virtual ~ListBase(){};
        bool isEnabled = true;;
        bool isExpanded = true;;
        uint32_t id = 0;
        QString name;
        uint16_t uiIndex = 0;
        EEntityType type = et_list;
        EInclusionMode inclusionMode = im_exclude;
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
        uint32_t list_id = 0;
        ECrossoverInclusionMode crossoverInclusionMode = cim_select_all;
        ECrossoverInclusionMode Rotate(ECrossoverInclusionMode mode){
            if(mode == cim_select_all)
                return cim_select_crossovers;
            if(mode == cim_select_crossovers)
                return cim_select_pure;
            return cim_select_all;
        };
    };

    struct FandomSearchStateToken{
        uint32_t id;
        EInclusionMode inclusionMode = im_exclude;
        ECrossoverInclusionMode crossoverInclusionMode = cim_select_all;
    };

}
}
