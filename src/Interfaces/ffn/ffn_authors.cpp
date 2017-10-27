#include "Interfaces/ffn/ffn_authors.h"

namespace database {

bool FFNAuthors::EnsureId(QSharedPointer<core::Author> author)
{
    return DBAuthorsBase::EnsureId(author, "ffn");
}
bool FFNAuthors::RemoveAuthor(QSharedPointer<core::Author> author)
{
    return DBAuthorsBase::RemoveAuthor(author, "ffn");
}
}
