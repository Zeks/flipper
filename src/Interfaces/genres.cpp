#include "Interfaces/genres.h"
#include "pure_sql.h"

namespace interfaces {

bool interfaces::Genres::IsGenreList(QStringList list)
{
    bool success = false;
    for(auto token : list)
        if(genres.contains(token))
        {
            success = true;
            break;
        }
    return success;
}

bool Genres::LoadGenres()
{
    genres = database::puresql::GetAllGenres(db);
    if(genres.empty())
        return false;
    return true;
}


}
