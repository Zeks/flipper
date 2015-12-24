#ifndef SIMPLESETTINGS_H_
#define SIMPLESETTINGS_H_
#include <functional>
#include <QSettings>
#include <QVariant>
#include <QTextCodec>

namespace SS
{
inline QVariant value(QString filename, QString valueName, QVariant defaultValue = QVariant(), bool useUtf8 = true)
{
    QSettings file(filename, QSettings::IniFormat);
    if(useUtf8)
		file.setIniCodec(QTextCodec::codecForName("UTF-8"));
	return file.value(valueName, defaultValue);
}

struct Holder
{
    Holder(QString filename, bool useUtf8 = true) : file(filename, QSettings::IniFormat)
	{
		if(useUtf8)
			file.setIniCodec(QTextCodec::codecForName("UTF-8"));
	};
	inline QVariant value(QString valueName, QVariant defaultValue = QVariant())
	{
		return file.value(valueName, defaultValue);	
	}
	QSettings file;
	bool useUtf8 = true;
};

}


#endif
