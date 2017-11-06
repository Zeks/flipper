#include "Interfaces/base.h"
#include "Interfaces/authors.h"
#include "section.h"
#include "QScopedPointer"
#include "QSharedPointer"
#include "QSqlDatabase"
#include "QReadWriteLock"


namespace interfaces {
class FFNAuthors : public Authors
{
public:
    virtual ~FFNAuthors();
    bool EnsureId(core::AuthorPtr) override;
    //bool RemoveAuthor(core::AuthorPtr author) override;

    // DBAuthorsBase interface
//public:
//    bool EnsureAuthor(int);
};

}
