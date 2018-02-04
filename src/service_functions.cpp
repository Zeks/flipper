/*
Flipper is a replacement search engine for fanfiction.net search results
Copyright (C) 2017  Marchenko Nikolai

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>
*/
#include "service_functions.h"
#include "url_utils.h"
#include "pagegetter.h"
#include "parsers/ffn/ficparser.h"
namespace service{

//    void ReprocessFic(core::Fic fic, ECacheMode cacheMode)
//    {
//        An<PageManager> pager;
//        QString url = url_utils::GetUrlFromWebId(fic.webId, fic.webSite);
//        auto page = pager->GetPage(url, cacheMode);
//        if(!page.isValid)
//            return;
//        FicParser parser;
//        parser.ProcessPage(url, page.content);
//    }
}
