#include "service_functions.h"
#include "url_utils.h"
#include "pagegetter.h"
#include "ficparser.h"
namespace service{

    void ReprocessFic(core::Fic fic, ECacheMode cacheMode)
    {
        An<PageManager> pager;
        QString url = url_utils::GetUrlFromWebId(fic.webId, fic.webSite);
        auto page = pager->GetPage(url, cacheMode);
        if(!page.isValid)
            return;
        FicParser parser;
        parser.ProcessPage(url, page.content);
    }
}
