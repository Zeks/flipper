#include "Interfaces/base.h"
#include "Interfaces/authors.h"
#include "section.h"
#include "QScopedPointer"
#include "QSharedPointer"
#include "QSqlDatabase"
#include "QReadWriteLock"


namespace database {
class FFNAuthors : public DBAuthorsBase
{
public:
    virtual ~FFNAuthors(){}
    bool EnsureId(QSharedPointer<core::Author>) override;
    bool RemoveAuthor(QSharedPointer<core::Author> author) override;
};

}
