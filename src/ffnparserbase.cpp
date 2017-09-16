#include "include/ffnparserbase.h"
#include <QDebug>

void FFNParserBase::ProcessGenres(core::Section &section, QString genreText)
{
    section.result.SetGenres(genreText, "ffn");
    qDebug() << section.result.genres;
}

void FFNParserBase::ProcessCharacters(core::Section &section, QString characters)
{
    section.result.charactersFull = characters.trimmed();
    qDebug() << characters;
}
