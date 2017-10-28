#include "Interfaces/tags.h"
#include "pure_sql.h"

namespace interfaces {

bool Tags::DeleteTag(QString tag)
{
    return database::puresql::DeleteTagfromDatabase(tag, db);
}

QStringList Tags::ReadUserTags()
{
    QStringList tags = database::puresql::ReadUserTags(db);
    if(tags.empty())
    {
        tags = CreateDefaultTagList();
        database::puresql::PushTaglistIntoDatabase(tags,db);
    }
    return tags;
}

QStringList Tags::CreateDefaultTagList()
{
    QStringList temp;
    temp << "smut" << "hidden" << "meh_description" << "unknown_fandom" << "read_queue" << "reading" << "finished" << "disgusting" << "crap_fandom";
    return temp;
}

}
