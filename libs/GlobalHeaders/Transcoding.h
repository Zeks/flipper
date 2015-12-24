#ifndef TRANSCODING_H
#define TRANSCODING_H
#include <QString>
#include <QStringList>
#include <QHash>
#include <QObject>
#include <QTextCodec>

enum ETranslationDirection
{
    to_rus = 0,
    to_lat = 1
};

static inline QString RecodeToLAT(QString text)
{
    //qDebug() << "recoding";
  //QTextCodec::setCodecForTr(QTextCodec::codecForName("UTF-8"));
  static QString rus = QObject::tr("АБВГДЕЖЗИЙКЛМНОПРСТУФХЦЧШЩЬЪЭЮЯЫ");
  static QString lat = "ABWGDEVZIJKLMNOPRSTUFHC4ШQXXЭЮQY";
  for(int i = 0; i < text.length(); ++i)
  {
      static int index;
	  index = rus.indexOf(text[i]);
      if(index  != -1)
          text[i]=lat.at(index);
  }

  return text;
}



static inline QString RecodeToRUS(QString text)
{
  //QTextCodec::setCodecForTr(QTextCodec::codecForName("UTF-8"));
  static QString rus = QObject::tr("АБВГДЕЖЗИЙКЛМНОПРСТУФХЦ4ШЩЬЪЭЮЯЫ");
  static QString lat = "ABWGDEVZIJKLMNOPRSTUFHC4ШQXXЭЮQY";
  for(int i = 0; i < text.length(); ++i)
  {
      static int index;
	  index = lat.indexOf(text[i]);
      if(index  != -1)
          text[i]=rus.at(index);
  }
  return text;
}
static inline QStringList RecodeToRUS(QStringList texts)
{
    QStringList result;
    for(auto value : texts)
    {
        QStringList split = value.split(" ");
        for(auto bit : split)
        {
            result.append(RecodeToRUS(bit));
        }
        result.append(RecodeToRUS(value));
    }
    result.removeDuplicates();
    return result;
}

static inline bool isRus(QString text)
{
    //QTextCodec::setCodecForTr(QTextCodec::codecForName("UTF-8"));
	static QRegExp rx("[" + QObject::tr("АБВГДЕЖЗИЙКЛМНОПРСТУФХЦЧШЩЬЪЭЮЯЫ") + "]");
    if(rx.indexIn(text)!= -1)
        return true;
    return false;
}

#endif // TRANSCODING_H

