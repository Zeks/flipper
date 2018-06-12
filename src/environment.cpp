#include "include/environment.h"
#include "include/tasks/fandom_task_processor.h"
#include "include/page_utils.h"
#include "include/Interfaces/recommendation_lists.h"
#include "include/Interfaces/fandoms.h"


void CoreEnvironment::InitMetatypes()
{
    qRegisterMetaType<WebPage>("WebPage");
    qRegisterMetaType<PageResult>("PageResult");
    qRegisterMetaType<ECacheMode>("ECacheMode");
    qRegisterMetaType<FandomParseTask>("FandomParseTask");
    qRegisterMetaType<FandomParseTaskResult>("FandomParseTaskResult");
}

void CoreEnvironment::Init()
{
    InitMetatypes();

    std::unique_ptr<core::DefaultRNGgenerator> rng (new core::DefaultRNGgenerator());
    rng->portableDBInterface = interfaces.db;
    queryBuilder.SetIdRNGgenerator(rng.release());

    QSettings settings("settings.ini", QSettings::IniFormat);
    auto storedRecList = settings.value("Settings/currentList").toString();
    interfaces.recs->SetCurrentRecommendationList(interfaces.recs->GetListIdForName(storedRecList));

    interfaces.recs->LoadAvailableRecommendationLists();
    interfaces.fandoms->FillFandomList(true);
}
