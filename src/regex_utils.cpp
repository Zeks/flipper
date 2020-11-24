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
#include "include/regex_utils.h"
#include <QRegularExpression>
#include <QRegularExpressionMatch>
#include <QString>
#include <QDebug>
#include "core/section.h"
#include <iostream>
NarrowResult GetNextInstanceOf(QString text, QString regex1, QString regex2, bool forward)
{
    QRegExp rx1(regex1);
    rx1.setMinimal(true);
    QRegExp rx2(regex2);
    rx2.setMinimal(true);
    int first = -1;
    int second = -1;
    int secondCandidate = -1;
    auto invalid = NarrowResult(-1,-1);
    if(regex1.isEmpty())
        first = text.length();
    else
        first = rx1.indexIn(text);
    if(first == -1)
        return invalid;

    if(forward)
    {
        second = rx2.indexIn(text, first);
        return {first,second};
    }
    else
    {
        if(regex1.isEmpty())
            first = text.length();
        do
        {
            second = secondCandidate;
            secondCandidate = rx2.indexIn(text, second + 1);
            if(secondCandidate == -1)
                break;
        }while(secondCandidate < first);
        if(second == -1)
            return invalid;
        return {first,second};
    }
}
QString GetSingleNarrow(QString text,QString regex1, QString regex2, bool forward1)
{
    QString result;
    auto firstNarrow = GetNextInstanceOf(text,regex1, regex2, forward1);
    result = text.mid(firstNarrow.first,firstNarrow.Length());
    return result;
}


QString GetDoubleNarrow(QString text,
                        QString regex1, QString regex2, bool forward1,
                        QString regex3, QString regex4, bool ,
                        int lengthOfLastTag)
{
    QString result;
    auto firstNarrow = GetNextInstanceOf(text,regex1, regex2, forward1);

    QString temp = text.mid(firstNarrow.first,firstNarrow.Length());
    auto secondNarrow = GetNextInstanceOf(temp,regex3,regex4, false);
    if(!firstNarrow.IsValid() || !secondNarrow.IsValid() )
        return result;
    result = temp.mid(secondNarrow.second + lengthOfLastTag, firstNarrow.second);
    return result;
}
RegularExpressionToken operator"" _s ( const char* data, size_t )
{
    return RegularExpressionToken(data, 0);
}
RegularExpressionToken operator"" _c ( const char* data, size_t )
{
    return RegularExpressionToken(data, 1);
}

QString BouncingSearch(QString str, FieldSearcher finder)
{
    auto tokens = finder.tokens;
    QString reversed = str;
    std::reverse(reversed.begin(), reversed.end());
    QStringRef directRef(&str);
    QStringRef reversedRef(&reversed);
    QStringRef currentString;
    //int lastPosition = 0;
    //Q_UNUSED(lastPosition);
    int skip = 0;
    bool found = true;
    int originalSize;
    for(const auto& token: tokens)
    {
        QRegularExpression rx;
        if(token.forwardDirection)
        {
            currentString = directRef;
            rx.setPattern(token.regex);
            //lastPosition = reversedRef.size() - skip;
        }
        else{
            currentString = reversedRef;
            rx.setPattern(token.reversedRegex);
            //lastPosition = skip;
        }
        auto match = rx.match(currentString);
        if(match.isValid())
        {
            skip = match.capturedStart() + token.moveAmount;
            originalSize = reversedRef.size();
            if(token.forwardDirection)
            {
                if(token.snapLeftBound)
                {
                    directRef = directRef.mid(skip);
                    reversedRef = reversedRef.mid(0, (originalSize - skip));
                }
                else
                {
                    directRef = directRef.mid(0, skip);
                    reversedRef = reversedRef.mid(originalSize - skip);
                }
            }
            else
            {
                if(token.snapLeftBound)
                {
                    reversedRef = reversedRef.mid(skip);
                    directRef = directRef.mid(0, (originalSize - skip));
                }
                else
                {
                    reversedRef = reversedRef.mid(0, skip);
                    directRef = directRef.mid(originalSize - skip);
                }
            }
        }
        else
        {
            found = false;
            break;
        }
    }
    QString result;
    if(found)
    {
        result = directRef.toString();
    }
    return result;
}

void CommonRegex::Init()
{
    universalSlashRegex = "(slash([^a-z]|$))|(\\smm(\\s|[,]))|(yaoi)|(lgbt)|((\\s|^|[.,])m[-/*x]m(\\s|$|[,.]))|(sho[u]{0,1}nen[\\s-]ai)|(\\sgay(\\s|[.]))|"
                          "(queer)|(mpreg)|(boy\\slove)|((boy|guy)\\s{0,1}x\\s{0,1}(boy|guy))|(homosexual)|(00q)|((\\s|[.])(k|s|^)[-/*x&]{1}(k|s)(\\s|[.]|$))|"
                          "(\\stop[!][a-z])|(\\sbottom[!][a-z])";

    QString characterSeparator = "((\\s{0,1}[-/\\*x]{1}\\s{0,1})|([-/\\*x]{0,1}))";
    QString names = "((harry)|(hp)|(cedric)|(lv)|(lucius)|(lm)|(ss)|(snape)|(sev(erus){0,1})"
                    "|(draco)|(dm)|(sirius)|(sb)|(remus)|(rl)|(ron)|(rw)|(t[m]{0,1}r)|(voldemort)"
                    "|(voldie)|(fenrir))";
    //QString shortenedNames = "((hp)|(lv)|(lm)|(ss)|(dm)|(sb)|(rl)|(rw)|(t[m]{0,1}r))";
    QString fixedNamesOnly = names + characterSeparator + names;
    QString fixedHpTerms = "(snarry)|(harrymort)|(drarry)";
    QString fixedHpFull = fixedHpTerms + "|(" + fixedNamesOnly + ")";
    slashRegexPerFandom["Harry Potter"] = fixedHpFull;



    names = "((kaka(shi){0,1})|(harry)|(sas(u){0,1}(ke){0,1})|(nar(u){0,1}(to){0,1})|(iru(ka){0,1})|(lee)|(gaa(ra){0,1})|(kiba)|(shika(maru){0,1})|(mina(to){0,1}))";
    fixedNamesOnly = names + characterSeparator + names;
    slashRegexPerFandom["Naruto"] = fixedNamesOnly;

    names = "((ichi(go){0,1})|(ren(ji){0,1})|(zara(ki){0,1})|(hitsugaya)|(ikka(ku){0,1})|(byaku(ya){0,1})|(chad))";
    fixedNamesOnly = names + characterSeparator + names;
    slashRegexPerFandom["Bleach"] = fixedNamesOnly;
    slashRegexPerFandom["Death Note"] = "(L/Light)|(Light/L)";
    slashRegexPerFandom["Detective Conan/Case Closed"] ="KaiShin|ShinKai";

    characterSlashPerFandom["Naruto"] = "([\\[]Naruto\\sU[.][,]\\sSasuke\\sU[.][\\]])";
    characterSlashPerFandom["Naruto"] += "|([\\[]Kakashi\\sH[.][,]\\sIruka\\sU[.][\\]])";
    characterSlashPerFandom["Thor"] = "(Iron\\sMan/Tony\\sS[.][,]\\sLoki[\\]])|(Thor[,]\\sLoki[\\]])";
    characterSlashPerFandom["Avengers"] = "(Iron\\sMan/Tony\\sS[.][,]\\sLoki[\\]])|(Thor[,]\\sLoki[\\]])";
    characterSlashPerFandom["Hobbit"] ="([\\[]Thorin[,]\\Bilbo\\sB[.][\\]])";



    notSlash = "((no[tn]{0,1}|isn[']t)(\\s|-){0,1}(a(\\s{0,1})){0,1}(slash|yaoi))|((\\s)jack\\sslash)|(fem\\w{0,}[!]{1})|(naruko)|(\\sfem\\s)|(\\sfem-)|(fem(m){0,1}(e){0,1}slash)|(\\smentor\\s)"
               "|(f/f)|Gen[.]|Geass\\s{0,}[/\\\\|]\\s{0,}Harry";

    QString notSlashCharacterSpecialCase;
    notSlashCharacterSpecialCase+="|(naru(to){0,1}\\s{0,1}" + characterSeparator + "\\s{0,1}naru(to){0,1})";
    notSlashCharacterSpecialCase+="|(harry\\s{0,1}" + characterSeparator + "\\s{0,1}harry)";
    notSlashCharacterSpecialCase+="|(ichigo\\s{0,1}" + characterSeparator + "\\s{0,1}ichigo)";
    notSlashCharacterSpecialCase+="|femnaru|femharry|femichi";
    notSlashCharacterSpecialCase+="|kagsess|inukag|sesskag|kaginu|hieikag|kaghiei";
    notSlashCharacterSpecialCase+="|femshep";
    notSlash+=notSlashCharacterSpecialCase;

    smut = "(\\srape)|(harem)|(smut)|(lime)|(\\ssex)|(dickgirl)|(shemale)|(nsfw)|(porn)|"
           "(futanari)|(lemon)|(yuri)|(incest)|(succubus)|(incub)|(\\sanal\\s)|(vagina)";

    characterSlashPerFandom["D.Gray-Man"] ="([\\[]Allen\\sWalker[,]\\sKanda\\sYuu[\\]])";

    characterNotSlashPerFandom["Harry Potter"] = "(Hermione)|(Bella)|(lily)|(winky)|(Luna\\sL)|(Fem[-]Harry)";
    characterNotSlashPerFandom["Inuyasha"] = "(Kagome)|(Rin)";
    characterNotSlashPerFandom["Naruto"] = "(Destroyer\\sof\\sWorlds)|(Sakura)|(Kushina)|(Tsunade)|(Temari)|(Ino\\sY)|(Hanabi)|(Mikoto)|"
                                           "(NaruHina)|(Eru\\sLee)|(Anko\\sM)|(Karin)|(Konan)(KakaRin)|(Hinata)|(Tenten)";
    characterNotSlashPerFandom["Mass Effect"] = "(Shepard\\s[\\()]F)|(Liara)";
    characterNotSlashPerFandom["Code Geass"] = "(Kallen/Zero)|(C[.]C[.])|(Nunnaly)|(Euphemia)|(Cornelia)";
    characterNotSlashPerFandom["RWBY"] = "(Velvet)|(Ruby\\sR)";
    characterNotSlashPerFandom["Worm"] = "(Skitter)";
    characterNotSlashPerFandom["Star Wars"] = "(Leia)|(Amidala)";
    characterNotSlashPerFandom["Twilight"] = "(Bella)";
    characterNotSlashPerFandom["Sword Art Online"] = "(Asuna)";
    characterNotSlashPerFandom["Evangelion"] = "(Yui\\sI)|(Asuka)";
    characterNotSlashPerFandom["Death Note"] = "(Misa)";
    characterNotSlashPerFandom["One Piece"] = "(Vivi)|(Nami)|(Boa Hancock)";
    characterNotSlashPerFandom["Gundam Wing/AC"] = "(Relena)";
    characterNotSlashPerFandom["Sailor Moon"] = "(Usagi)";
    characterNotSlashPerFandom["Bleach"] = "(Nemu)|(Rukia)|(Yachiru)|(Matsumoto)|(Orihime)|(Karin)|(Yuzu\\sK)|(Hinamori)|(Tatsuki)";
    characterNotSlashPerFandom["Fairy Tail"] = "(Lucy\\sH)|(Erza)";
    characterNotSlashPerFandom["Fullmetal Alchemist"] = "Riza\\sH";
    characterNotSlashPerFandom["Hellsing"] = "(Seras)|(Integra)";


    rxUniversal.setPattern(universalSlashRegex);
    rxUniversal.setPatternOptions(QRegularExpression::CaseInsensitiveOption | QRegularExpression::InvertedGreedinessOption);

    rxNotSlash.setPattern(notSlash);
    rxNotSlash.setPatternOptions(QRegularExpression::CaseInsensitiveOption | QRegularExpression::InvertedGreedinessOption);

    rxSmut.setPattern(smut);
    rxSmut.setPatternOptions(QRegularExpression::CaseInsensitiveOption | QRegularExpression::InvertedGreedinessOption);


    for(auto i = slashRegexPerFandom.cbegin(); i != slashRegexPerFandom.cend(); i++)
    {
        rxHashSlashFandom[i.key()].setPattern(i.value());
        rxHashSlashFandom[i.key()].setPatternOptions(QRegularExpression::CaseInsensitiveOption | QRegularExpression::InvertedGreedinessOption);
    }

    for(auto i = characterSlashPerFandom.cbegin(); i != characterSlashPerFandom.cend(); i++)
    {
        rxHashCharacterSlashFandom[i.key()].setPattern(i.value());
        rxHashCharacterSlashFandom[i.key()].setPatternOptions(QRegularExpression::CaseInsensitiveOption | QRegularExpression::InvertedGreedinessOption);
    }

    for(auto i = characterNotSlashPerFandom.cbegin(); i != characterNotSlashPerFandom.cend(); i++)
    {
        rxHashCharacterNotSlashFandom[i.key()].setPattern(i.value());
        rxHashCharacterNotSlashFandom[i.key()].setPatternOptions(QRegularExpression::CaseInsensitiveOption | QRegularExpression::InvertedGreedinessOption);
    }
    initComplete = true;
}

void CommonRegex::Log()
{
//    QHash<QString, QString> slashRegexPerFandom;
//    QHash<QString, QString> characterSlashPerFandom;

//    QString universalSlashRegex;
//    QString notSlash; // universal not slash
//    QString smut; // universal smut
    qDebug().noquote() << "Logging regex token";
    qDebug().noquote() << "Smut: " << smut;
    qDebug().noquote() << "Not slash: " << notSlash;
    qDebug().noquote() << "US: " << universalSlashRegex;

    for(auto i =  slashRegexPerFandom.cbegin(); i != slashRegexPerFandom.cend(); i++)
        qDebug().noquote() << "FS: " << i.key() << " " << i.value();
    for(auto i =  characterSlashPerFandom.cbegin(); i != characterSlashPerFandom.cend(); i++)
        qDebug().noquote() << "FCS: " << i.key() << " " << i.value();

}

// apply per fandom summary token
//    for(auto fandom : fandoms)
//    {
//        if(slashRegexPerFandom.contains(fandom))
//            containsSlash = containsSlash  || summary.contains(QRegularExpression(slashRegexPerFandom[fandom], QRegularExpression::CaseInsensitiveOption));

//        if(characterSlashPerFandom.contains(fandom))
//            containsSlash = containsSlash  || characters.contains(QRegularExpression(characterSlashPerFandom[fandom], QRegularExpression::CaseInsensitiveOption));
//    }

SlashPresence CommonRegex::ContainsSlash(QString summary, QString characters, QString fandoms) const
{
    SlashPresence result;
    bool containsSlash = false;
    // apply universal regex
    bool doLogging = false;
    bool doNotSlashCharacterLogging = false;
    QRegularExpressionMatch match;
    match = rxUniversal.match(summary);
    containsSlash = match.hasMatch();

    if(doLogging && containsSlash)
    {
        qDebug().noquote() << summary;
        qDebug().noquote() << match.capturedTexts();
        qDebug().noquote() << "end match";
    }


    for(auto i = slashRegexPerFandom.cbegin(); i != slashRegexPerFandom.cend(); i++)
    {
        QRegularExpressionMatch match;
        if(fandoms.contains(i.key()))
        {
            match = rxHashSlashFandom[i.key()].match(summary);
        }
        containsSlash = containsSlash || match.hasMatch();
//        if(containsSlash)
//        {
//            qDebug() << rxHashSlashFandom[fandom].pattern();
//            qDebug() << match.capturedTexts();
//        }
        if(doLogging && match.hasMatch())
        {
            qDebug().noquote() << i.value();
            qDebug().noquote() << summary;
            qDebug() << match.capturedTexts();
            qDebug() << "end match";
        }
    }

    for(auto i = characterSlashPerFandom.cbegin(); i != characterSlashPerFandom.cend(); i++)
    {
        QRegularExpressionMatch match;
        if(fandoms.contains(i.key()))
        {
            match = rxHashCharacterSlashFandom[i.key()].match(characters);
        }
        if(doLogging && match.hasMatch())
        {
            qDebug().noquote() << summary;
            qDebug() << match.capturedTexts();\
            qDebug() << "end match";
        }
        containsSlash = containsSlash || match.hasMatch();
    }
    result.containsSlash = containsSlash;

    bool containsNotSlash = false;

    match = rxNotSlash.match(summary);
    containsNotSlash = match.hasMatch() || characters.contains("Minerva");
    if(doLogging && match.hasMatch())
    {
        qDebug().noquote() << summary;
        qDebug() << match.capturedTexts();\
        qDebug() << "end match";
    }

    for(auto i = characterNotSlashPerFandom.cbegin(); i != characterNotSlashPerFandom.cend(); i++)
    {
        QRegularExpressionMatch match;
        if(fandoms.contains(i.key()))
        {
            match = rxHashCharacterNotSlashFandom[i.key()].match(characters);
        }
        if(doNotSlashCharacterLogging && match.hasMatch())
        {
            qDebug().noquote() << summary;
            qDebug() << match.capturedTexts();\
            qDebug() << "end match";
        }
        containsNotSlash = containsNotSlash || match.hasMatch();
    }
    result.containsNotSlash = containsNotSlash;

    return result;
}

QString GetSlashRegex()
{
    static QString slashRx = "(slash)|(\\smm\\s)|(yaoi)|(lgbt)|(m[-/*x]m)|(harry[-/*x]{0,1}cedric)|(lv[-/*x]{0,1}hp)|(hp[-/*x]{0,1}lv)|"
                             "(hp[-/*x]{0,1}ss)|(ss[-/*x]{0,1}hp)|(harrymort)|(homosexual)|(harry[-/*x]draco)|(draco[-/*x]harry)|"
                             "(ulquiorra[-/*x]ichigo)|(ichigo[-/*x]ulquiorra)|"
                             "(ichi(go){0,1}[-/*x]ren(ji){0,1})|(ren(ji){0,1}[-/*x]ichi(go){0,1})|"
                             "(ichi(go){0,1}[-/*x]byak)|(byak(uya){0,1}[-/*x]ichi(go){0,1})|"
                             "(hp[-/*x]{0,1}dm)|(dm[-/*x]{0,1}hp)|(mpreg)|(sasu[-/*x]{0,1}naru)|(snarry)|"
                             "(naru[-/*x]{0,1}sasu)|(sho[u]{0,1}nen\\sai)|(ita[-/*x]{0,1}naru)|(snape[-/*x]{0,1}harry)|"
                             "(naru[-/*x]{0,1}ita)|(kaka[-/*x]{0,1}iru)|(iru[-/*x]{0,1}kaka)|(kaka[-/*x]{0,1}naru)|"
                             "(naru[-/*x]{0,1}kaka)|(kaka[-/*x]{0,1}sasu)|(sasu[-/*x]{0,1}kaka)(kiba[-/*x]{0,1}naru)|"
                             "(naru[-/*x]{0,1}kiba)|(naru[-/*x]{0,1}iru)|(iru[-/*x]{0,1}naru)(gaara[-/*x]{0,1}naru)|"
                             "(naru[-/*x]{0,1}gaara)|(\\sgay(\\s|[.]))|(queer)|(boy\\slove)|"
                             "(voldemort\\s{0,2}/\\s{0,2}harry)|(harry/voldemort)|(drarry)|(hp[/]{0,1}t[m]{0,1}r)|(t[m]{0,1}r[/]{0,1}hp)|"
                             "(rw[-/*x]{0,1}hp)|(hp[-/*x]{0,1}rw)|(hp[-/*x]{0,1}lm)|(lm[-/*x]{0,1}hp)|"
                             "(sb[-/*x]{0,1}hp)|(hp[-/*x]{0,1}sb)|(rl[-/*x]{0,1}hp)|(hp[-/*x]{0,1}rl)|(k[-/*x]s|s[-/*x]k)"
                             "([\\[]Naruto\\sU[.][,]\\sSasuke\\sU[.][\\]])|"
                             "(Iron\\sMan/Tony\\sS[.][,]\\sLoki[\\]])|(00q)|"
                             "(Thor[,]\\sLoki[\\]])"
                             "([\\[]Thorin[,]\\Bilbo\\sB[.][\\]])|((boy|guy)\\s{0,1}x\\s{0,1}(boy|guy))";

    return slashRx;
}

