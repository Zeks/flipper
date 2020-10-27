#pragma once

#include <string>
#include <memory>

namespace core{
namespace fandom_lists{

    struct ListBase{
        bool isEnabled;
        uint16_t uiIndex;
    };

    struct List : public ListBase{
        using ListPtr = std::shared_ptr<List>;
        bool isDefault;
        bool isInverted;
        uint16_t listId;
        std::string listName;
    };


    enum EFandomInclusionMode{
        fim_include_fandom = 0,
        fim_exclude_fandom = 1,
    };

    enum ECrossoverInclusionMode{
        cim_select_all = 0,
        cim_select_pure = 1,
        cim_select_crossovers = 2,
    };

    struct FandomStateInList: public ListBase{
        uint32_t fandom_id;
        uint32_t list_id;
        EFandomInclusionMode fandomInclusionMode;
        ECrossoverInclusionMode crossoverInclusionMode;
    };

}
}
