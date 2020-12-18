#pragma once
#include <string>
namespace sql{

enum class ESqlErrors{
    se_unknown = -1,
    se_none = 0,
    se_duplicate_column = 1,
    se_unique_row_violation = 2,
    se_no_data_to_be_fetched = 3,
};

class Error{
public:
    Error(){}
    Error(std::string str, ESqlErrors errorType){
        errortext = str;
        if(!errortext.empty())
            hasError = true;
        actualErrorType = errorType;
    }
    std::string text() const{return errortext;};
    bool isValid() const{return hasError;};
    ESqlErrors getActualErrorType() const{return actualErrorType;}

private:
    std::string errortext;
    ESqlErrors actualErrorType;
    bool hasError = false;
};





}
