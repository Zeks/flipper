#ifndef _STRUCTTODBEDITOR_H
#define _STRUCTTODBEDITOR_H


#include <QString>
#include <QDebug>
#include <QHash>
#include <QVariant>
#include <QStringList>

#include "l_tree_controller_global.h"

#include <functional>
#include "logger/QsLog.h"
#include "fsqldatabase.h"
#include "fsqlquery.h"
#include "fsqlerror.h"
#include <QSqlError>
#include <QSqlField>
#include <QSqlRecord>


template<class T>
class  StructToDBEditor  
{
  public:
    bool Insert(T * data, FSqlDatabase db);

    bool UpdateIndexAsLastInTable(T * data, FSqlDatabase db, std::function<void(T*,int)> indexSetter);

    bool Update(T * data, FSqlDatabase db);

    bool Delete(T * data, FSqlDatabase db);

    bool InsertOrUpdate( T * data, FSqlDatabase db);

    bool TestExistence(T * data, FSqlDatabase db);

    void SetTableName(QString name){tableName = name;}

     QHash<QString, std::function<bool(T*)> > nameToPermission;
     QHash<QString, std::function<QVariant(T*)> > nameToValue;
     QStringList keys;
     QString tableName;
};
template<class T>
bool StructToDBEditor<T>::Insert(T * data, FSqlDatabase db)
{
    // Bouml preserved body begin 0021AA2A
    QString insertString = "Insert into %1";
    insertString=insertString.arg(tableName);
    QString columnsPart;
    QString valuesPart;
    for(QString key: nameToValue.keys())
    {
        bool allowBinding = true;
        if(nameToPermission.contains(key))
            allowBinding = nameToPermission[key](data);
        if(allowBinding)
        {
            columnsPart+=key + " , ";
            valuesPart+="\'" + nameToValue[key](data).toString() + "\' " + " , ";
        }
    }
    if(!columnsPart.isEmpty())
    {
        columnsPart.chop(3);
    }
    if(!valuesPart.isEmpty())
    {
        valuesPart.chop(3);
    }
    columnsPart = QString(" (") + columnsPart + QString(") ");
    valuesPart = QString(" VALUES (") + valuesPart + QString(") ");
    QString fullString = insertString + columnsPart + valuesPart;
    FSqlQuery q(fullString, db);
    q.next();
    if(q.lastError().isValid())
    {
       QLOG_FATAL() << q.lastError().text();
       return false;
    }
    return true;
    // Bouml preserved body end 0021AA2A
}

template<class T>
bool StructToDBEditor<T>::UpdateIndexAsLastInTable(T * data, FSqlDatabase db, std::function<void(T*,int)> indexSetter)
{
    // Bouml preserved body begin 00221D2A
    if(keys.size() == 0)
    {
        QLOG_WARN() << "Attempted to get new index but keys are empty";
        return false;
    }
    QString indexGetterString = "Select max(%1) from %2";
    indexGetterString=indexGetterString.arg(keys.at(0)).arg(tableName);
    FSqlQuery q(indexGetterString, db);
    q.next();
    if(q.lastError().isValid())
    {
       QLOG_FATAL() << q.lastError().text();
       return false;
    }
    indexSetter(data, q.value(0).toInt());
    return true;
    // Bouml preserved body end 00221D2A
}

template<class T>
bool StructToDBEditor<T>::Update(T * data, FSqlDatabase db)
{
    // Bouml preserved body begin 0021AC2A
    QString updateString = "UPDATE %1 set ";
    updateString=updateString.arg(tableName);
    QString wherePart;
    QString valuesPart;
    for(QString key: nameToValue.keys())
    {
        bool allowBinding = true;
        if(nameToPermission.contains(key) && !keys.contains(key))
            allowBinding = nameToPermission[key](data);
        if(allowBinding)
        {
            valuesPart+=key + " = " + "\'" + nameToValue[key](data).toString() + "\'" + ", ";
        }
    }

    for(QString key : keys)
    {
        wherePart+= " " + key + " = " + "\'" + nameToValue[key](data).toString() + "\'" + " && ";
    }

    if(!wherePart.isEmpty())
    {
        wherePart.chop(4);
    }
    if(!valuesPart.isEmpty())
    {
        valuesPart.chop(2);
    }

    QString fullString = updateString + valuesPart + " where " + wherePart;
    FSqlQuery q(fullString, db);
    q.next();
    if(q.lastError().isValid())
    {
       QLOG_FATAL() << q.lastError().text();
       return false;
    }
    return true;
    // Bouml preserved body end 0021AC2A
}

template<class T>
bool StructToDBEditor<T>::Delete(T * data, FSqlDatabase db)
{
    // Bouml preserved body begin 0021ADAA
    if(!TestExistence(data, db))
        return false;
    QString updateString = "Delete from %1 where ";
    updateString=updateString.arg(tableName);
    QString wherePart;

    for(QString key : keys)
    {
        QString temp;
        if(key == "ID")
            temp = QString::number(nameToValue[key](data).toInt());
        else
            temp = nameToValue[key](data).toString();
        wherePart+= " " + key + " = " + temp + " && ";
    }

    if(!wherePart.isEmpty())
    {
        wherePart.chop(4);
    }


    QString fullString = updateString + wherePart;
    FSqlQuery q(fullString, db);
    bool exists = q.next();
    if(q.lastError().isValid())
    {
       QLOG_FATAL() << q.lastError().text();
       return false;
    }
    return exists;

    // Bouml preserved body end 0021ADAA
}

template<class T>
bool StructToDBEditor<T>::InsertOrUpdate(T * data, FSqlDatabase db)
{
    // Bouml preserved body begin 0021AD2A
    if(TestExistence( data, db))
        return Update( data, db);
    else
        return Insert( data, db);
    // Bouml preserved body end 0021AD2A
}

template<class T>
bool StructToDBEditor<T>::TestExistence(T * data, FSqlDatabase db)
{
    // Bouml preserved body begin 0021ACAA
    QString updateString = "select * from %1 where ";
    updateString=updateString.arg(tableName);
    QString wherePart;

    for(QString key : keys)
    {
        QString temp;
        if(key == "ID")
            temp = QString::number(nameToValue[key](data).toInt());
        else
            temp = nameToValue[key](data).toString();
        wherePart+= " " + key + " = " + temp + " && ";
    }

    if(!wherePart.isEmpty())
    {
        wherePart.chop(4);
    }


    QString fullString = updateString + wherePart;
    FSqlQuery q(fullString, db);
    bool exists = q.next();
    if(q.lastError().isValid())
    {
       QLOG_FATAL() << q.lastError().text();
       return false;
    }
    return exists;
    // Bouml preserved body end 0021ACAA
}

#endif
