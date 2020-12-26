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
    Error(std::string str, std::string query, ESqlErrors errorType){
        errorText = str;
        queryText = query;
        if(!errorText.empty())
            hasError = true;
        actualErrorType = errorType;
    }
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
    int errorCode = 0;
    bool hasError = false;
};





}
