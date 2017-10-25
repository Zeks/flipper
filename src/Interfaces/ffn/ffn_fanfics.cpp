
#include "Interfaces/ffn/ffn_fanfics.h"

namespace database {



int FFNFanfics::GetIDFromWebID(int value)
{
    ::GetIDFromWebID(value, "ffn");
}

int FFNFanfics::GetWebIDFromID(int value)
{
    ::GetWebIDFromID(value, "ffn");
}

bool FFNFanfics::DeactivateFic(int ficId)
{
    return puresql::DeactivateStory(ficId, "ffn", db);
}

}
