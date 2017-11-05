#pragma once
#include <algorithm>

template <typename T, template <typename> class Cont>
void CleanupPtrList(Cont<T>& container){
    auto it = std::remove_if(std::begin(container), std::end(container), [](auto item){
                                 return !item;});
    container.erase(it, std::end(container));
}
