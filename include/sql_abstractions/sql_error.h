#pragma once
#include <string>
namespace sql{
class Error{
public:
    Error(){}
    Error(std::string str){errortext = str; hasError = true;}
    std::string text() const{return errortext;};
    bool isValid() const{return hasError;};

private:
    std::string errortext;
    bool hasError = false;
};

}
