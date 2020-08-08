#include "core/fanfic.h"
#include "core/section.h"


namespace core {
Fic::Fic(){
    author = QSharedPointer<Author>(new Author);
}

}
