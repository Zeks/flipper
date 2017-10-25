#include "Interfaces/tags.h"

namespace database {

bool Tags::DeleteTag(QString tag)
{
    puresql::DeleteTagfromDatabase(tag, db);
}


}
