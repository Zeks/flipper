#include "Interfaces/ffn/ffn_authors.h"

namespace interfaces {

FFNAuthors::~FFNAuthors()
{

}

bool FFNAuthors::EnsureId(QSharedPointer<core::Author> author)
{
    return Authors::EnsureId(author, "ffn");
}
bool FFNAuthors::RemoveAuthor(QSharedPointer<core::Author> author)
{
    return Authors::RemoveAuthor(author, "ffn");
}

//bool FFNAuthors::EnsureAuthor(int id)
//{

//}
}
