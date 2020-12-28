/*
Flipper is a recommendation and search engine for fanfiction.net
Copyright (C) 2017-2020  Marchenko Nikolai

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
#pragma once
#include <string>
namespace sql{

enum class ESqlErrors{
    se_generic_sql_error = -1,
    se_none = 0,
    se_duplicate_column = 1,
    se_unique_row_violation = 2,
    se_no_data_to_be_fetched = 3,
    se_broken_connection = 4,
};

class Error{
public:
    Error(){}
    // constructor for sql operations
    Error(std::string str, std::string query, ESqlErrors errorType){
        errorText = str;
        queryText = query;
        if(!errorText.empty())
            hasError = true;
        actualErrorType = errorType;
    }
    // constructor for db specific errors like failure opening transaction
    Error(std::string str, ESqlErrors errorType){
        errorText = str;
        if(!errorText.empty())
            hasError = true;
        actualErrorType = errorType;
    }
    std::string text() const{return errorText;};
    std::string query() const{return queryText;};
    bool isValid() const{return hasError;};
    ESqlErrors getActualErrorType() const{return actualErrorType;}

private:
    std::string errorText;
    std::string queryText;
    ESqlErrors actualErrorType;
    bool hasError = false;
};





}
