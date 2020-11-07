#pragma once
#include <string>
namespace sql{
class Error{
public:
    std::string text() const;
};

}
