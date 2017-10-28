#include "Interfaces/base.h"
#include "Interfaces/fanfics.h"
#include "section.h"
#include "QScopedPointer"
#include "QSharedPointer"
#include "QSqlDatabase"
#include "QReadWriteLock"


namespace interfaces {

class FFNFanfics : public DBFanficsBase{
public:
    virtual ~FFNFanfics(){}
    virtual int GetIDFromWebID(int);
    virtual int GetWebIDFromID(int);
    virtual bool DeactivateFic(int ficId);
    virtual int GetIdForUrl(QString url);
    void ProcessIntoDataQueues(QList<QSharedPointer<core::Fic>> fics, bool alwaysUpdateIfNotInsert = false);
    void AddRecommendations(QList<core::FicRecommendation> recommendations);
};
}
