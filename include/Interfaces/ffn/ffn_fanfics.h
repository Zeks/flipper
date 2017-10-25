#include "Interfaces/base.h"
#include "Interfaces/fanfics.h"
#include "section.h"
#include "QScopedPointer"
#include "QSharedPointer"
#include "QSqlDatabase"
#include "QReadWriteLock"


namespace database {

class FFNFanfics : public DBFanficsBase{
public:
    virtual ~FFNFanfics(){}
    virtual int GetIDFromWebID(int);
    virtual int GetWebIDFromID(int);
    virtual bool DeactivateFic(int ficId);

    void ProcessIntoDataQueues(QList<QSharedPointer<core::Fic>> fics, bool alwaysUpdateIfNotInsert = false);
    void AddRecommendations(QList<core::FicRecommendation> recommendations);
};
}
