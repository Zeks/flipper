#include "Interfaces/tags.h"
#include "pure_sql.h"

namespace database {

bool Tags::DeleteTag(QString tag)
{
    return puresql::DeleteTagfromDatabase(tag, db);
}


}
