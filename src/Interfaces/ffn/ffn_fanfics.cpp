
#include "Interfaces/ffn/ffn_fanfics.h"
#include "pure_sql.h"

namespace interfaces {



int FFNFanfics::GetIDFromWebID(int value)
{
    return DBFanficsBase::GetIDFromWebID(value, "ffn");
}

int FFNFanfics::GetWebIDFromID(int value)
{
    return DBFanficsBase::GetWebIDFromID(value, "ffn");
}

bool FFNFanfics::DeactivateFic(int ficId)
{
    return database::puresql::DeactivateStory(ficId, "ffn", db);
}

int FFNFanfics::GetIdForUrl(QString url)
{
    return -1; //! todo
}

}
