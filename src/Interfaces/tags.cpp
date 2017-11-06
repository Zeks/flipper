#include "Interfaces/tags.h"
#include "pure_sql.h"

namespace interfaces {

void Tags::LoadAlltags()
{
    allTags = ReadUserTags();
}

bool Tags::DeleteTag(QString tag)
{
    return database::puresql::DeleteTagFromDatabase(tag, db);
}

bool Tags::CreateTag(QString tag)
{
    return database::puresql::CreateTagInDatabase(tag, db);
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

bool Tags::SetTagForFic(int ficId, QString tag)
{
    tag = tag.trimmed();
    if(!allTags.contains(tag))
        CreateTag(tag);
    allTags.push_back(tag);
    database::puresql::AssignTagToFanfic(tag, ficId, db);
    return true;
}

bool Tags::RemoveTagFromFic(int ficId, QString tag)
{
    return database::puresql::RemoveTagFromFanfic(tag, ficId, db);
}

QStringList Tags::CreateDefaultTagList()
{
    QStringList temp;
    temp << "smut" << "hidden" << "meh_description" << "unknown_fandom" << "read_queue" << "reading" << "finished" << "disgusting" << "crap_fandom";
    return temp;
}

}
