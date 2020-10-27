#pragma once

#include <string>
#include <memory>

namespace core{
namespace fandom_lists{

    struct ListBase{
        bool isEnabled;
        uint16_t uiIndex;
    };

    enum EInclusionMode{
        im_include = 0,
        im_exclude = 1,
    };

    struct List : public ListBase{
        using ListPtr = std::shared_ptr<List>;
        bool isDefault;
        EInclusionMode inclusionMode;
        uint16_t listId;
        QString listName; // otherwise I will have to convert on display
    };




    enum ECrossoverInclusionMode{
        cim_select_all = 0,
        cim_select_pure = 1,
        cim_select_crossovers = 2,
    };

    struct FandomStateInList: public ListBase{
        uint32_t fandom_id;
        uint32_t list_id;
        QString name; // otherwise I will have to convert on display
        EInclusionMode inclusionMode;
        ECrossoverInclusionMode crossoverInclusionMode;
    };

}
}
