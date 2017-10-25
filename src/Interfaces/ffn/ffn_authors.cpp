#include "Interfaces/ffn/ffn_authors.h"

namespace database {

bool FFNAuthors::EnsureId(QSharedPointer<core::Author> author)
{
    ::EnsureId(author, "ffn");
}
void  FFNAuthors::RemoveAuthor(QSharedPointer<core::Author> author)
{
    ::RemoveAuthor(author, "ffn");
}
}
