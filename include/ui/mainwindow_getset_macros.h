#pragma once

#define ADD_STRING_GETSET(HOLDER,ROW,ROLE,PARAM)  \
    HOLDER->AddGetter(QPair<int,int>(ROW,ROLE), \
    [] (const core::Fanfic* data) \
{ \
    if(data) \
    return QVariant(data->PARAM); \
    else \
    return QVariant(); \
    } \
    ); \
    HOLDER->AddSetter(QPair<int,int>(ROW,ROLE), \
    [] (core::Fanfic* data, QVariant value) \
{ \
    if(data) \
    data->PARAM = value.toString(); \
    } \
    )


#define ADD_STRING_NUMBER_GETSET(HOLDER,ROW,ROLE,PARAM)  \
    HOLDER->AddGetter(QPair<int,int>(ROW,ROLE), \
    [] (const core::Fanfic* data) \
{ \
    if(data){ \
    QString temp;\
    QSettings settings("settings/ui.ini", QSettings::IniFormat);\
    settings.setIniCodec(QTextCodec::codecForName("UTF-8"));\
    if(settings.value("Settings/commasInWordcount", false).toBool()){\
    QLocale::setDefault(QLocale(QLocale::English, QLocale::UnitedStates));\
    QLocale aEnglish;\
    temp = aEnglish.toString(data->PARAM.toInt());}\
    else{ \
    temp = data->PARAM;} \
    return QVariant(temp);} \
    else \
    return QVariant(); \
    } \
    ); \
    HOLDER->AddSetter(QPair<int,int>(ROW,ROLE), \
    [] (core::Fanfic* data, QVariant value) \
{ \
    if(data) \
    data->PARAM = value.toString(); \
    } \
    ) \


#define ADD_DATE_GETSET(HOLDER,ROW,ROLE,PARAM)  \
    HOLDER->AddGetter(QPair<int,int>(ROW,ROLE), \
    [] (const core::Fanfic* data) \
{ \
    if(data) \
    return QVariant(data->PARAM); \
    else \
    return QVariant(); \
    } \
    ); \
    HOLDER->AddSetter(QPair<int,int>(ROW,ROLE), \
    [] (core::Fanfic* data, QVariant value) \
{ \
    if(data) \
    data->PARAM = value.toDateTime(); \
    } \
    ) \

#define ADD_STRING_INTEGER_GETSET(HOLDER,ROW,ROLE,PARAM)  \
    HOLDER->AddGetter(QPair<int,int>(ROW,ROLE), \
    [] (const core::Fanfic* data) \
{ \
    if(data) \
    return QVariant(data->PARAM); \
    else \
    return QVariant(); \
    } \
    ); \
    HOLDER->AddSetter(QPair<int,int>(ROW,ROLE), \
    [] (core::Fanfic* data, QVariant value) \
{ \
    if(data) \
    data->PARAM = QString::number(value.toInt()); \
    } \
    ) \

#define ADD_INTEGER_GETSET(HOLDER,ROW,ROLE,PARAM)  \
    HOLDER->AddGetter(QPair<int,int>(ROW,ROLE), \
    [] (const core::Fanfic* data) \
{ \
    if(data) \
    return QVariant(data->PARAM); \
    else \
    return QVariant(); \
    } \
    ); \
    HOLDER->AddSetter(QPair<int,int>(ROW,ROLE), \
    [] (core::Fanfic* data, QVariant value) \
{ \
    if(data) \
    data->PARAM = value.toInt(); \
    } \
    ) \

#define ADD_ENUM_GETSET(GETTER, SETTER, HOLDER,ROW,ROLE,PARAM)  \
    HOLDER->AddGetter(QPair<int,int>(ROW,ROLE), \
    [] (const core::Fanfic* data) \
{ \
    if(data) \
    return QVariant(GETTER(data->PARAM)); \
    else \
    return QVariant(); \
    } \
    ); \
    HOLDER->AddSetter(QPair<int,int>(ROW,ROLE), \
    [] (core::Fanfic* data, QVariant value) \
{ \
    if(data) \
    SETTER(data, value.toInt());    \
    } \
    ) \

#define ADD_STRINGLIST_GETTER(HOLDER,ROW,ROLE,PARAM)  \
    HOLDER->AddGetter(QPair<int,int>(ROW,ROLE), \
    [] (const core::Fanfic* data) \
{ \
    if(data) \
    return QVariant(data->PARAM); \
    else \
    return QVariant(); \
    } \
    ) \

