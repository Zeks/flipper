/*
FFSSE is a replacement search engine for fanfiction.net search results
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
#include "Interfaces/ffn/ffn_authors.h"

namespace interfaces {

FFNAuthors::~FFNAuthors()
{

}

bool FFNAuthors::EnsureId(QSharedPointer<core::Author> author)
{
    return Authors::EnsureId(author, "ffn");
}
//bool FFNAuthors::RemoveAuthor(core::AuthorPtr author)
//{
//    return Authors::RemoveAuthor(author, "ffn");
//}

//bool FFNAuthors::EnsureAuthor(int id)
//{

//}
}
